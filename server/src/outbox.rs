use std::sync::{Arc, Condvar, Mutex};

#[derive(Default)]
struct Output {
    snapshot: Option<Arc<[u8]>>,
    result: Option<Arc<[u8]>>,
    closed: bool,
    result_taken: bool,
}

/// One unsent snapshot per connection; the result follows the last snapshot.
/// Publication never waits for socket I/O.
#[derive(Default)]
pub(crate) struct Outbox {
    state: Mutex<Output>,
    ready: Condvar,
}

impl Outbox {
    pub(crate) fn result_taken(&self) -> bool {
        self.state.lock().unwrap().result_taken
    }

    pub(crate) fn publish(&self, snapshot: Arc<[u8]>) {
        let mut state = self.state.lock().unwrap();
        if state.closed || state.result.is_some() { return; }
        state.snapshot = Some(snapshot);
        self.ready.notify_one();
    }

    pub(crate) fn finish(&self, result: Arc<[u8]>) {
        let mut state = self.state.lock().unwrap();
        if state.closed || state.result.is_some() { return; }
        state.result = Some(result);
        self.ready.notify_one();
    }

    pub(crate) fn close(&self) {
        let mut state = self.state.lock().unwrap();
        state.closed = true;
        state.snapshot = None;
        state.result = None;
        self.ready.notify_one();
    }

    pub(crate) fn take(&self) -> Option<Arc<[u8]>> {
        let mut state = self.state.lock().unwrap();
        loop {
            if state.closed { return None; }
            if let Some(snapshot) = state.snapshot.take() { return Some(snapshot); }
            if let Some(result) = state.result.take() {
                state.closed = true;
                state.result_taken = true;
                return Some(result);
            }
            state = self.ready.wait(state).unwrap();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn slow_writer_keeps_only_latest_snapshot_then_result() {
        let output = Outbox::default();
        for tick in 0..10_000u32 {
            output.publish(Arc::from(tick.to_le_bytes()));
        }
        output.finish(Arc::from(&b"result"[..]));
        output.publish(Arc::from([99u8]));
        assert_eq!(&*output.take().unwrap(), &9999u32.to_le_bytes());
        assert_eq!(&*output.take().unwrap(), b"result");
        assert!(output.take().is_none());
        assert!(output.result_taken());
    }

    #[test]
    fn close_discards_pending_snapshot_and_wakes_writer() {
        let output = Arc::new(Outbox::default());
        let receiver = output.clone();
        let writer = std::thread::spawn(move || receiver.take());
        output.close();
        assert!(writer.join().unwrap().is_none());
        output.publish(Arc::from([1u8]));
        output.finish(Arc::from(&b"result"[..]));
        assert!(output.take().is_none());
        let pending = Outbox::default();
        pending.publish(Arc::from([2u8]));
        pending.close();
        assert!(pending.take().is_none());
        assert!(!pending.result_taken());
    }
}
