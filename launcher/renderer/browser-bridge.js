// Browser bridge for viewing Rage Studio in a standard web browser.
// When running inside Electron, window.launcher is already frozen by preload.cjs and this is a no-op.
if (!window.launcher) {
  const listeners = new Set();

  async function callApi(method, args) {
    try {
      const res = await fetch('/api/' + encodeURIComponent(method), {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({args})
      });
      if (res.ok) return await res.json();
    } catch (_) {}
    return {ok: false, error: 'Preview server unreachable'};
  }

  window.launcher = new Proxy({
    onState(listener) {
      listeners.add(listener);
      return () => listeners.delete(listener);
    }
  }, {
    get(target, prop) {
      if (prop in target) return target[prop];
      if (prop === 'openStudio') {
        return async kind => {
          const urls = {
            car: '/car-studio.html',
            track: '/track-studio.html',
            saves: '/index.html?page=saves',
            settings: '/index.html?page=settings',
            classic: '/index.html?page=settings'
          };
          window.location.href = urls[kind] || '/index.html';
          return {ok: true, value: {studio: kind}};
        };
      }
      return (...args) => callApi(String(prop), args);
    }
  });
}
