//! Pure packet encoding; no sockets, clocks, rooms or simulation stepping.
use crate::{Plan, SEAT_COUNT, FIELD_COUNT, AI_COUNT, sim};

pub(crate) const PROTOCOL_VERSION: u8 = 26;
const POSE_SIZE: usize = 1 + 19 * 4;
const START_SIZE: usize = 27 + SEAT_COUNT * 6 + 16 + AI_COUNT * 7;
const HASH_OFFSET: usize = 27 + SEAT_COUNT * 6;

// Wire protocol (see docs/multiplayer.md, "Wire messages"). Small and
// binary, one TCP connection per client, little-endian throughout.
pub(crate) mod wire {
    pub const C2S_HELLO: u8 = 0x01;
    pub const C2S_INPUT: u8 = 0x02;
    pub const C2S_LOADED: u8 = 0x03;
    pub const C2S_READY: u8 = 0x04;
    pub const C2S_PICK: u8 = 0x05;
    pub const C2S_RACE: u8 = 0x06;
    pub const C2S_ROOM: u8 = 0x07;
    pub const C2S_LIST: u8 = 0x08;
    pub const C2S_COMMAND: u8 = 0x09;

    pub const S2C_WELCOME: u8 = 0x81;
    pub const S2C_START: u8 = 0x82;
    pub const S2C_SNAPSHOT: u8 = 0x83;
    pub const S2C_RESULT: u8 = 0x84;
    pub const S2C_LIST: u8 = 0x85;
    pub const S2C_LOBBY: u8 = 0x86;
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct RoomInfo {
    pub code: u64,
    pub options: [u8; 4],
    pub occupied: u8,
    pub ready: u8,
    // 0 open, 1 preparing, 2 racing. Finished rooms are reaped, not advertised.
    pub state: u8,
}

#[derive(Default, Clone, Debug, PartialEq, Eq)]
pub(crate) struct LobbySeat {
    pub variant: u8, pub manual: bool, pub length: u8, pub name: [u8; 15],
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub(crate) struct Lobby { pub room: RoomInfo, pub seats: [LobbySeat; SEAT_COUNT] }

pub(crate) fn lobby_message(lobby: &Lobby) -> Option<[u8; 53]> {
    if !valid_room(&lobby.room) { return None; }
    let mut bytes = [0; 53];
    bytes[..2].copy_from_slice(&[wire::S2C_LOBBY, PROTOCOL_VERSION]);
    bytes[2..17].copy_from_slice(&room_record(&lobby.room));
    for (index, seat) in lobby.seats.iter().enumerate() {
        if seat.length > 15 || seat.variant as u32 >= sim::CAR_MODEL_VARIANT_COUNT ||
            seat.name[seat.length as usize..].iter().any(|byte| *byte != 0) { return None; }
        if lobby.room.occupied & (1 << index) == 0 &&
            (seat.variant != 0 || seat.manual || seat.length != 0) { return None; }
        let offset = 17 + index * 18;
        bytes[offset..offset + 3].copy_from_slice(&[seat.variant, seat.manual as u8, seat.length]);
        bytes[offset + 3..offset + 18].copy_from_slice(&seat.name);
    }
    Some(bytes)
}

fn valid_room(room: &RoomInfo) -> bool {
    let options = sim::MpRaceOptions { classIndex: room.options[0], course: room.options[1],
        laps: room.options[2], reverse: room.options[3] };
    room.code > 0 && room.code <= i64::MAX as u64 && room.occupied & !3 == 0 &&
        room.ready & !room.occupied == 0 && room.state <= 2 &&
        unsafe { sim::MpValidRaceOptions(&options) } != 0
}

fn room_record(room: &RoomInfo) -> [u8; 15] {
    let mut bytes = [0; 15];
    bytes[..8].copy_from_slice(&room.code.to_le_bytes());
    bytes[8..12].copy_from_slice(&room.options);
    bytes[12..].copy_from_slice(&[room.occupied, room.ready, room.state]);
    bytes
}

pub(crate) fn room_list(rooms: &[RoomInfo]) -> Option<Vec<u8>> {
    if rooms.len() > crate::ROOM_LIMIT { return None; }
    let mut bytes = Vec::with_capacity(3 + rooms.len() * 15);
    bytes.extend_from_slice(&[wire::S2C_LIST, PROTOCOL_VERSION, rooms.len() as u8]);
    for (index, room) in rooms.iter().enumerate() {
        if !valid_room(room) || rooms[..index].iter().any(|other| other.code == room.code) { return None; }
        bytes.extend_from_slice(&room_record(room));
    }
    Some(bytes)
}

pub(crate) fn welcome_message(seat: usize, room: u64) -> Option<[u8; 11]> {
    if seat >= SEAT_COUNT || room == 0 || room > i64::MAX as u64 { return None; }
    let mut bytes = [0; 11];
    bytes[..3].copy_from_slice(&[wire::S2C_WELCOME, PROTOCOL_VERSION, seat as u8]);
    bytes[3..].copy_from_slice(&room.to_le_bytes());
    Some(bytes)
}

pub(crate) fn start_message(plan: &Plan, boot: [u8; 16], fingerprint: u64, executable: u64) -> Option<[u8; START_SIZE]> {
    if !plan.valid() || boot[0] == 0 || !boot.contains(&0) || executable == 0 { return None; }
    let mut message = [0; START_SIZE];
    message[..6].copy_from_slice(&[wire::S2C_START, PROTOCOL_VERSION, plan.course as u8,
        plan.class as u8, plan.laps as u8, plan.reverse as u8]);
    message[6..10].copy_from_slice(&plan.countdown.to_le_bytes());
    message[10] = SEAT_COUNT as u8;
    message[11..27].copy_from_slice(&boot);
    for (entry, seat) in message[27..HASH_OFFSET].chunks_exact_mut(6).zip(&plan.seats) {
        entry[..2].copy_from_slice(&[seat.model as u8, seat.manual as u8]);
        entry[2..].copy_from_slice(&seat.seed.to_le_bytes());
    }
    message[HASH_OFFSET..HASH_OFFSET + 8].copy_from_slice(&fingerprint.to_le_bytes());
    message[HASH_OFFSET + 8..HASH_OFFSET + 16].copy_from_slice(&executable.to_le_bytes());
    for (i, rival) in plan.rivals.iter().enumerate() {
        if let Some(rival) = rival {
            let offset = HASH_OFFSET + 16 + i * 7;
            message[offset..offset + 3].copy_from_slice(&[1, rival.model, rival.slot]);
            message[offset + 3..offset + 7].copy_from_slice(&rival.seed.to_le_bytes());
        }
    }
    Some(message)
}

pub(crate) fn pose_message(pose: sim::CarPose) -> [u8; POSE_SIZE] {
    let mut message = [0; POSE_SIZE];
    message[0] = pose.status;
    for (index, value) in [pose.x, pose.y, pose.z, pose.yaw, pose.pitch, pose.roll,
                  pose.steering, pose.wheels, pose.brake, pose.progress, pose.rpm, pose.throttle,
                  pose.clutch, pose.gear, pose.ground, pose.roll_speed, pose.speed, pose.lap, pose.place].iter().enumerate() {
        message[1 + index * 4..5 + index * 4].copy_from_slice(&value.to_le_bytes());
    }
    message
}

pub(crate) const SNAPSHOT_SIZE: usize = 10 + FIELD_COUNT * POSE_SIZE + SEAT_COUNT * 4;

pub(crate) fn snapshot_message(tick: u32, elapsed: u32, phase: u8, poses: [sim::CarPose; FIELD_COUNT], acknowledged: [u32; SEAT_COUNT]) -> [u8; SNAPSHOT_SIZE] {
    let mut message = [0; SNAPSHOT_SIZE];
    message[0] = wire::S2C_SNAPSHOT;
    message[1..5].copy_from_slice(&tick.to_le_bytes());
    message[5..9].copy_from_slice(&elapsed.to_le_bytes());
    message[9] = phase;
    for (seat, pose) in poses.into_iter().enumerate() {
        message[10 + seat * POSE_SIZE..10 + (seat + 1) * POSE_SIZE].copy_from_slice(&pose_message(pose));
    }
    for (seat, sequence) in acknowledged.into_iter().enumerate() {
        let offset = 10 + FIELD_COUNT * POSE_SIZE + seat * 4;
        message[offset..offset + 4].copy_from_slice(&sequence.to_le_bytes());
    }
    message
}

pub(crate) fn encode_result(results: [(bool, u8, i32); SEAT_COUNT]) -> [u8; 1 + SEAT_COUNT * 6] {
    let mut message = [0; 1 + SEAT_COUNT * 6];
    message[0] = wire::S2C_RESULT;
    for (seat, (finished, place, time)) in results.into_iter().enumerate() {
        let offset = 1 + seat * 6;
        message[offset] = finished as u8;
        message[offset + 1] = place;
        message[offset + 2..offset + 6].copy_from_slice(&time.to_le_bytes());
    }
    message
}

#[cfg(test)]
mod tests {
    #[test]
    fn lobby_packet_preserves_names_and_choices_through_c_decoding() {
        let lobby = super::Lobby { room: super::RoomInfo { code: 42, options: [5, 3, 6, 1],
            occupied: 2, ready: 2, state: 0 }, seats: [super::LobbySeat::default(),
                super::LobbySeat { variant: 31, manual: true, length: 15, name: *b"123456789012345" }] };
        let packet = super::lobby_message(&lobby).unwrap();
        assert_eq!(&packet[..17], &[0x86, 26, 42, 0, 0, 0, 0, 0, 0, 0, 5, 3, 6, 1, 2, 2, 0]);
        assert_eq!(&packet[17..35], &[0; 18]);
        assert_eq!(&packet[35..38], &[31, 1, 15]);
        let mut decoded = crate::sim::MpLobby::default();
        assert_eq!(unsafe { crate::sim::MpDecodeLobby(packet.as_ptr(), packet.len(), &mut decoded) }, 1);
        assert_eq!(decoded.room.code, 42);
        assert_eq!((decoded.seats[1].variant, decoded.seats[1].manual, decoded.seats[1].length), (31, 1, 15));
        assert_eq!(decoded.seats[1].name[..15].iter().map(|byte| *byte as u8).collect::<Vec<_>>(), b"123456789012345");
        assert_eq!(decoded.seats[1].name[15], 0);
        let mut invalid = lobby.clone();
        invalid.seats[1].length = 16;
        assert!(super::lobby_message(&invalid).is_none());
        invalid = lobby.clone(); invalid.seats[1].length = 14;
        assert!(super::lobby_message(&invalid).is_none());
        invalid = lobby.clone(); invalid.seats[1].variant = 32;
        assert!(super::lobby_message(&invalid).is_none());
        invalid = lobby.clone(); invalid.room.occupied = 0; invalid.room.ready = 0;
        assert!(super::lobby_message(&invalid).is_none());
    }
    #[test]
    fn room_directory_has_bounded_canonical_packets() {
        use super::{RoomInfo, room_list};
        assert_eq!(room_list(&[]).unwrap(), [0x85, 26, 0]);
        let room = RoomInfo { code: 0x0102030405060708, options: [5, 3, 6, 1],
            occupied: 3, ready: 2, state: 1 };
        assert_eq!(room_list(&[room]).unwrap(),
            [0x85, 26, 1, 8, 7, 6, 5, 4, 3, 2, 1, 5, 3, 6, 1, 3, 2, 1]);
        let encoded = room_list(&[room]).unwrap();
        let mut decoded = [crate::sim::MpRoomInfo::default(); crate::ROOM_LIMIT];
        let mut count = 99;
        assert_eq!(unsafe { crate::sim::MpDecodeRoomList(encoded.as_ptr(), encoded.len(),
            decoded.as_mut_ptr(), &mut count) }, 1);
        assert_eq!(count, 1);
        assert_eq!(decoded[0].code, room.code);
        assert_eq!([decoded[0].options.classIndex, decoded[0].options.course,
            decoded[0].options.laps, decoded[0].options.reverse], room.options);
        assert_eq!((decoded[0].occupied, decoded[0].ready, decoded[0].state), (3, 2, 1));
        for invalid in [RoomInfo { code: 0, ..room }, RoomInfo { code: u64::MAX, ..room },
            RoomInfo { occupied: 4, ..room }, RoomInfo { occupied: 1, ready: 2, ..room },
            RoomInfo { state: 3, ..room }, RoomInfo { options: [0, 0, 0, 0], ..room }] {
            assert!(room_list(&[invalid]).is_none());
        }
        assert!(room_list(&[room, room]).is_none());
        let rooms: Vec<_> = (1..=crate::ROOM_LIMIT + 1).map(|code| RoomInfo { code: code as u64, ..room }).collect();
        assert_eq!(room_list(&rooms[..crate::ROOM_LIMIT]).unwrap().len(), 3 + crate::ROOM_LIMIT * 15);
        assert!(room_list(&rooms).is_none());
    }
    #[test]
    fn welcome_identifies_the_assigned_room() {
        assert_eq!(super::welcome_message(1, 0x0102030405060708),
            Some([0x81, 26, 1, 8, 7, 6, 5, 4, 3, 2, 1]));
        assert!(super::welcome_message(2, 1).is_none());
        assert!(super::welcome_message(0, 0).is_none());
        assert!(super::welcome_message(0, u64::MAX).is_none());
    }
    #[test]
    fn start_message_matches_c_client_fixture() {
        let boot = *b"SCES_006.96\0\0\0\0\0";
        let mut expected = vec![0x82, 26, 0, 0, 3, 0, 150, 0, 0, 0, 2];
        expected.extend_from_slice(&boot);
        expected.extend_from_slice(&[0, 0, 0x70, 0x56, 0x34, 0x12,
                                    0, 0, 0x71, 0x56, 0x34, 0x12]);
        expected.extend_from_slice(&[0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01]);
        expected.extend_from_slice(&[8, 7, 6, 5, 4, 3, 2, 1]);
        expected.extend_from_slice(&[0; super::AI_COUNT * 7]);
        let packet = super::start_message(&super::Plan::default(), boot,
            0x0123456789ABCDEF, 0x0102030405060708).unwrap();
        assert_eq!(packet.as_slice(), expected.as_slice());
        let mut decoded = super::sim::MpStart::default();
        assert_ne!(unsafe { super::sim::MpDecodeStart(packet[1..].as_ptr(),
            packet.len() - 1, &mut decoded) }, 0);
        assert_eq!(decoded.boot.map(|byte| byte as u8), boot);
        assert_eq!(decoded.fingerprint, 0x0123456789ABCDEF);
        assert_eq!(decoded.executable, 0x0102030405060708);
        for unknown in [[0; 16], [b'X'; 16]] {
            assert!(super::start_message(&super::Plan::default(), unknown, 1, 2).is_none());
        }
        assert!(super::start_message(&super::Plan::default(), boot, 1, 0).is_none());
        let mut plan = super::Plan::default();
        plan.rivals[super::AI_COUNT - 1] = Some(crate::Rival { model: 10, slot: 10, seed: 42 });
        let packet = super::start_message(&plan, boot, 1, 2).unwrap();
        assert_eq!(&packet[packet.len() - 7..], &[1, 10, 10, 42, 0, 0, 0]);
        assert_ne!(unsafe { super::sim::MpDecodeStart(packet[1..].as_ptr(),
            packet.len() - 1, &mut decoded) }, 0);
        assert_eq!(decoded.rivals[super::AI_COUNT - 1].seed, 42);
        plan.rivals[0] = plan.rivals[super::AI_COUNT - 1];
        assert!(super::start_message(&plan, boot, 1, 2).is_none());
        plan.rivals[0] = Some(crate::Rival { model: 11, slot: 0, seed: 0 });
        assert!(super::start_message(&plan, boot, 1, 2).is_none());
    }

    #[test]
    fn presentation_pose_matches_c_decoder_layout() {
        let bytes = super::pose_message(super::sim::CarPose {
            status: 1, pitch: -1, roll: 1, steering: -80,
            wheels: 4096, brake: 256, progress: 99, rpm: 8000, throttle: 256, clutch: 2, gear: 3, ground: -600, roll_speed: -7, speed: -1234, lap: 2, place: 1, ..Default::default()
        });
        let mut expected = [0; 77];
        expected[0] = 1;
        expected[17..21].copy_from_slice(&(-1i32).to_le_bytes());
        expected[21] = 1;
        expected[25..29].copy_from_slice(&(-80i32).to_le_bytes());
        expected[30] = 16;
        expected[34] = 1;
        expected[37] = 99;
        expected[41..45].copy_from_slice(&8000i32.to_le_bytes());
        expected[46] = 1;
        expected[49] = 2;
        expected[53] = 3;
        expected[57..61].copy_from_slice(&(-600i32).to_le_bytes());
        expected[61..65].copy_from_slice(&(-7i32).to_le_bytes());
        expected[65..69].copy_from_slice(&(-1234i32).to_le_bytes());
        expected[69..73].copy_from_slice(&2i32.to_le_bytes());
        expected[73..77].copy_from_slice(&1i32.to_le_bytes());
        assert_eq!(bytes, expected);
        let bytes = super::pose_message(super::sim::CarPose {
            status: 2, ..Default::default()
        });
        assert_eq!(bytes.len(), 77);
        assert_eq!(bytes[0], 2);
        assert!(bytes[1..].iter().all(|byte| *byte == 0));
    }

    #[test]
    fn result_matches_c_finish_and_retirement_fixture() {
        let bytes = super::encode_result([(true, 1, 1000), (false, 0, -1)]);
        assert_eq!(bytes, [0x84, 1, 1, 0xE8, 3, 0, 0, 0, 0, 255, 255, 255, 255]);
        for entries in [
            [(true, 1, 1000), (false, 0, -1)],
            [(false, 0, -1), (true, 1, i32::MAX)],
            [(true, 2, 1010), (true, 1, 1000)],
        ] {
            let packet = super::encode_result(entries);
            let mut decoded = super::sim::MpResult::default();
            assert_ne!(unsafe { super::sim::MpDecodeResult(packet[1..].as_ptr(),
                packet.len() - 1, &mut decoded) }, 0);
            for (seat, (finished, place, time)) in entries.into_iter().enumerate() {
                assert_eq!(decoded.seats[seat].finished != 0, finished);
                assert_eq!(decoded.seats[seat].place, place as i32);
                assert_eq!(decoded.seats[seat].milliseconds, time);
            }
        }
    }
}
