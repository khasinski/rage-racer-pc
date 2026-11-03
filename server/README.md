# Rage Racer HD origin

Compiled, read-only release server; see [distribution and publishing](../docs/hd-distribution.md).

```sh
build/release/rage-hd-publish PACK_DIRECTORY RELEASE_DIRECTORY https://YOUR_DOMAIN
HD_BIND=0.0.0.0 build/release/rage-hd-server RELEASE_DIRECTORY 3000
```

Terminate TLS at the existing reverse proxy. Serve the release directory read-only
and retain old immutable versions. No credentials or game/disc data are needed.
`docker build -t rage-hd server` builds the optional container.

Production: **https://rage-hd.polanka.ovh**, CapRover app `rage-hd`. The Docker
image was built/tested on ragnarok and the full published pack was downloaded
and validated by the compiled game downloader over HTTPS. Release files are at
`/home/ubuntu/projects/rage-hd-server/releases`. Preserve immutable version URLs;
update only the region catalog after validating each new release.
