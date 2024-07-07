Bun.serve({
  port: 8080,
  websocket: {
    message(ws, msg) {
      console.log(msg);
      ws.sendText(msg as string);
    },
  },
  fetch(req, server) {
    const { pathname } = new URL(req.url);
    if (pathname.startsWith('/ws')) {
      server.upgrade(req);
      return undefined;
    }
    return new Response('Hello', { status: 200 });
  },
});
