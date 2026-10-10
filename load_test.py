"""Load test: many concurrent clients each send messages and verify exact echo."""
import asyncio, time, sys

CLIENTS = int(sys.argv[1]) if len(sys.argv) > 1 else 500
MSGS = int(sys.argv[2]) if len(sys.argv) > 2 else 200
PORT = 9090

async def client(i, stats):
    r, w = await asyncio.open_connection("127.0.0.1", PORT)
    for m in range(MSGS):
        payload = f"c{i}-m{m}-".encode() + b"x" * 40 + b"\n"
        w.write(payload)
        await w.drain()
        echo = await r.readexactly(len(payload))
        if echo != payload:
            stats["bad"] += 1
        stats["ok"] += 1
    w.close()
    await w.wait_closed()

async def main():
    stats = {"ok": 0, "bad": 0}
    t0 = time.perf_counter()
    await asyncio.gather(*(client(i, stats) for i in range(CLIENTS)))
    dt = time.perf_counter() - t0
    print(f"clients={CLIENTS} msgs/client={MSGS} total={stats['ok']} mismatches={stats['bad']}")
    print(f"elapsed={dt:.2f}s throughput={stats['ok']/dt:,.0f} round-trips/s")

asyncio.run(main())
