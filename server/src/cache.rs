//! Immutable cache generation: publish archive and identity together by rename.
use std::{fs, io, path::{Path, PathBuf}, time::{SystemTime, UNIX_EPOCH}};
use super::sim;

const VERSION: u32 = 1;
const SIZE: usize = 52;

fn invalid() -> io::Error { io::Error::new(io::ErrorKind::InvalidData, "invalid server data cache") }

fn metadata(boot: [u8; 16], executable: u64, data: &[u8]) -> io::Result<Vec<u8>> {
    if boot[0] == 0 || !boot.contains(&0) || executable == 0 { return Err(invalid()); }
    let mut archive = std::mem::MaybeUninit::<sim::RaceData>::uninit();
    if unsafe { sim::ReadRaceData(data.as_ptr().cast(), data.len(), archive.as_mut_ptr()) } == 0 {
        return Err(invalid());
    }
    let fingerprint = unsafe { sim::ArchiveFingerprint(data.as_ptr().cast(), data.len()) };
    let mut bytes = b"RRCACHE\0".to_vec();
    bytes.extend_from_slice(&VERSION.to_le_bytes());
    bytes.extend_from_slice(&boot);
    bytes.extend_from_slice(&executable.to_le_bytes());
    bytes.extend_from_slice(&fingerprint.to_le_bytes());
    bytes.extend_from_slice(&(data.len() as u64).to_le_bytes());
    Ok(bytes)
}

pub struct Cache {
    pub boot: [u8; 16],
    pub executable: u64,
    pub fingerprint: u64,
    pub file: PathBuf,
}

pub fn read(path: &Path) -> io::Result<Cache> {
    let selected = match fs::read_to_string(path.join("current")) {
        Ok(name) => {
            let name = name.strip_suffix('\n').unwrap_or(&name);
            if !name.starts_with("data-") || name.len() > 100 ||
                !name.bytes().all(|byte| byte.is_ascii_hexdigit() || byte == b'-' || byte == b't') {
                return Err(invalid());
            }
            path.join(name)
        }
        Err(error) if error.kind() == io::ErrorKind::NotFound => path.to_path_buf(),
        Err(error) => return Err(error),
    };
    let path = selected.as_path();
    let bytes = fs::read(path.join("identity"))?;
    if bytes.len() != SIZE || &bytes[..8] != b"RRCACHE\0" ||
        bytes[8..12] != VERSION.to_le_bytes() { return Err(invalid()); }
    let boot = bytes[12..28].try_into().map_err(|_| invalid())?;
    let executable = u64::from_le_bytes(bytes[28..36].try_into().map_err(|_| invalid())?);
    let data = fs::read(path.join("RAGE.BIN"))?;
    if metadata(boot, executable, &data)? != bytes { return Err(invalid()); }
    let fingerprint = u64::from_le_bytes(bytes[36..44].try_into().map_err(|_| invalid())?);
    Ok(Cache { boot, executable, fingerprint, file: path.join("RAGE.BIN") })
}

/* Generations are immutable; only the current-generation pointer is replaced. */
pub fn write(path: &Path, boot: [u8; 16], executable: u64, data: &[u8]) -> io::Result<()> {
    let identity = metadata(boot, executable, data)?;
    let replacing = path.exists();
    if replacing && !path.is_dir() { return Err(invalid()); }
    let parent = path.parent().filter(|parent| !parent.as_os_str().is_empty()).unwrap_or(Path::new("."));
    let name = path.file_name().ok_or_else(invalid)?.to_string_lossy();
    let nonce = SystemTime::now().duration_since(UNIX_EPOCH).map_err(io::Error::other)?.as_nanos();
    let temporary = parent.join(format!(".{name}.{}.{nonce}.tmp", std::process::id()));
    fs::create_dir(&temporary)?;
    let result = (|| {
        for (name, bytes) in [("RAGE.BIN", data), ("identity", identity.as_slice())] {
            use std::io::Write;
            let mut file = fs::OpenOptions::new().write(true).create_new(true).open(temporary.join(name))?;
            file.write_all(bytes)?;
            file.sync_all()?;
        }
        #[cfg(unix)]
        fs::File::open(&temporary)?.sync_all()?;
        if !replacing { return fs::rename(&temporary, path); }
        let generation = format!("data-{}-{nonce}", std::process::id());
        fs::rename(&temporary, path.join(&generation))?;
        let pointer = path.join(format!(".current.{}.{nonce}.tmp", std::process::id()));
        let publish = (|| {
            use std::io::Write;
            let mut file = fs::OpenOptions::new().write(true).create_new(true).open(&pointer)?;
            writeln!(file, "{generation}")?;
            file.sync_all()?;
            drop(file);
            fs::rename(&pointer, path.join("current"))
        })();
        if publish.is_err() {
            let _ = fs::remove_file(&pointer);
            let _ = fs::remove_dir_all(path.join(&generation));
        }
        publish
    })();
    if result.is_err() { let _ = fs::remove_dir_all(&temporary); }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn complete_generation_roundtrips_and_failed_replacement_preserves_it() {
        let nonce = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos();
        let root = std::env::temp_dir().join(format!("rage-cache-{}-{nonce}", std::process::id()));
        fs::create_dir(&root).unwrap();
        let cache = root.join("data");
        let boot = *b"SCES_006.96\0\0\0\0\0";
        let archive = vec![0; 135 * 8];
        write(&cache, boot, 123, &archive).unwrap();
        assert_eq!(read(&cache).unwrap().boot, boot);
        assert_eq!(read(&cache).unwrap().executable, 123);
        assert_eq!(fs::read(cache.join("RAGE.BIN")).unwrap(), archive);
        let loaded = sim::RaceArchive::load_cache(&cache).unwrap();
        assert_eq!(loaded.boot(), boot);
        assert_eq!(loaded.executable(), 123);
        assert_eq!(loaded.fingerprint(), read(&cache).unwrap().fingerprint);
        assert!(write(&cache, boot, 0, &archive).is_err());
        assert_eq!(read(&cache).unwrap().executable, 123);
        assert!(write(&root.join("bad"), boot, 123, &[0]).is_err());
        assert_eq!(fs::read_dir(&root).unwrap().count(), 1);
        let mut identity = fs::read(cache.join("identity")).unwrap();
        identity[8] = 2;
        fs::write(cache.join("identity"), identity).unwrap();
        assert!(read(&cache).is_err());
        assert!(sim::RaceArchive::load_cache(&cache).is_none());
        identity = metadata(boot, 123, &archive).unwrap();
        for size in 0..SIZE {
            fs::write(cache.join("identity"), &identity[..size]).unwrap();
            assert!(read(&cache).is_err());
        }
        fs::write(cache.join("identity"), identity).unwrap();
        let mut changed = archive.clone();
        changed.push(0);
        fs::write(cache.join("RAGE.BIN"), changed).unwrap();
        assert!(read(&cache).is_err());
        assert!(sim::RaceArchive::load_cache(&cache).is_none());
        /* Repair creates a new generation; damaged old bytes remain untouched. */
        write(&cache, boot, 456, &archive).unwrap();
        assert_eq!(read(&cache).unwrap().executable, 456);
        assert_eq!(sim::RaceArchive::load_cache(&cache).unwrap().executable(), 456);
        assert_eq!(fs::read(cache.join("RAGE.BIN")).unwrap().len(), archive.len() + 1);
        let older = read(&cache).unwrap().file;
        let older_identity = fs::read(older.parent().unwrap().join("identity")).unwrap();
        write(&cache, boot, 789, &archive).unwrap();
        assert_eq!(read(&cache).unwrap().executable, 789);
        assert_eq!(fs::read(older.parent().unwrap().join("identity")).unwrap(), older_identity);
        let current = fs::read(cache.join("current")).unwrap();
        assert!(write(&cache, boot, 0, &archive).is_err());
        assert_eq!(fs::read(cache.join("current")).unwrap(), current);
        for name in ["../data-1", "/data-1", "data-1/other", "data-1\nextra"] {
            fs::write(cache.join("current"), name).unwrap();
            assert!(read(&cache).is_err());
        }
        fs::remove_file(cache.join("current")).unwrap();
        fs::create_dir(cache.join("current")).unwrap();
        let count = fs::read_dir(&cache).unwrap().count();
        assert!(write(&cache, boot, 999, &archive).is_err());
        assert_eq!(fs::read_dir(&cache).unwrap().count(), count);
        fs::remove_dir_all(root).unwrap();
    }
}
