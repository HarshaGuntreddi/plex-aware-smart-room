import os
import threading
import time
from flask import Flask, jsonify, request
from plexapi.server import PlexServer
import requests
from dotenv import load_dotenv

load_dotenv()

PLEX_BASEURL = os.getenv("PLEX_BASEURL", "").strip()
PLEX_TOKEN = os.getenv("PLEX_TOKEN", "").strip()
PLEX_CLIENT_NAME = os.getenv("PLEX_CLIENT_NAME", "").strip()
LIGHTS_HOST = os.getenv("LIGHTS_HOST", "").strip()
FAN_HOST = os.getenv("FAN_HOST", "").strip()
MOTION_SECRET = os.getenv("MOTION_SECRET", "").strip()

if not PLEX_BASEURL or not PLEX_TOKEN:
    raise SystemExit("set PLEX_BASEURL and PLEX_TOKEN in .env")

plex = PlexServer(PLEX_BASEURL, PLEX_TOKEN)

app = Flask(__name__)

state = {
    "playing": False,
    "last_title": None,
}

def http_get(url, timeout=2):
    try:
        r = requests.get(url, timeout=timeout)
        return r.status_code
    except Exception:
        return None

def trigger_cinema_mode(on: bool):
    if LIGHTS_HOST:
        endpoint = "dim" if on else "normal"
        http_get(f"http://{LIGHTS_HOST}/lights/{endpoint}")
    if FAN_HOST and not on:
        http_get(f"http://{FAN_HOST}/fan/off")

def any_video_playing():
    try:
        sessions = plex.sessions()
    except Exception:
        return False, None

    for s in sessions:
        if getattr(s, "TYPE", None) in ("movie", "episode"):
            for p in getattr(s, "players", []):
                st = getattr(p, "state", "").lower()
                if st == "playing":
                    title = getattr(s, "title", "unknown")
                    if PLEX_CLIENT_NAME:
                        name = getattr(p, "title", "") or getattr(p, "product", "")
                        if str(name).lower() != PLEX_CLIENT_NAME.lower():
                            continue
                    return True, title
    return False, None

def pause_all():
    try:
        sessions = plex.sessions()
    except Exception:
        return

    for s in sessions:
        for p in getattr(s, "players", []):
            if PLEX_CLIENT_NAME:
                name = getattr(p, "title", "") or getattr(p, "product", "")
                if str(name).lower() != PLEX_CLIENT_NAME.lower():
                    continue
            try:
                p.pause()
            except Exception:
                continue

@app.route("/api/is-playing", methods=["GET"])
def api_is_playing():
    playing, _ = any_video_playing()
    return jsonify({"playing": playing})

@app.route("/api/now-playing", methods=["GET"])
def api_now_playing():
    try:
        sessions = plex.sessions()
    except Exception:
        return jsonify({"playing": False, "items": []})

    items = []
    for s in sessions:
        kind = getattr(s, "TYPE", "unknown")
        title = getattr(s, "title", "unknown")
        grandparent = getattr(s, "grandparentTitle", "")
        parent = getattr(s, "parentTitle", "")
        for p in getattr(s, "players", []):
            st = getattr(p, "state", "").lower()
            if st not in ("playing", "paused", "buffering"):
                continue
            if PLEX_CLIENT_NAME:
                name = getattr(p, "title", "") or getattr(p, "product", "")
                if str(name).lower() != PLEX_CLIENT_NAME.lower():
                    continue
            item = {
                "type": kind,
                "title": title,
                "series": grandparent,
                "season": parent,
                "player": getattr(p, "title", ""),
                "state": st,
            }
            items.append(item)

    return jsonify({"playing": bool(items), "items": items})

@app.route("/api/pause-from-motion", methods=["POST"])
def api_pause_from_motion():
    payload = request.get_json(silent=True) or {}
    secret = str(payload.get("secret", ""))
    if not MOTION_SECRET or secret != MOTION_SECRET:
        return jsonify({"ok": False}), 403
    pause_all()
    return jsonify({"ok": True})

def monitor_loop():
    while True:
        playing, title = any_video_playing()
        if playing != state["playing"]:
            state["playing"] = playing
            state["last_title"] = title
            trigger_cinema_mode(playing)
        time.sleep(5)

def main():
    t = threading.Thread(target=monitor_loop, daemon=True)
    t.start()
    app.run(host="0.0.0.0", port=5000)

if __name__ == "__main__":
    main()
