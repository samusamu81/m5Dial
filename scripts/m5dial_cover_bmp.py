"""Copertina del player per l'M5Dial: scarica il JPEG da Music Assistant e lo salva come
BMP 24 bit non compresso 100x100 in /config/www/m5dial_cover.bmp (servito su /local/m5dial_cover.bmp).
Il dial non ha PSRAM e non riesce ad allocare il decoder JPEG: il BMP si decodifica con pochissima RAM.
Uso: python3 m5dial_cover_bmp.py <url>   (lanciato da shell_command.m5dial_cover_bmp)
"""
import io
import os
import sys
import urllib.request

from PIL import Image

OUT = "/config/www/m5dial_cover.bmp"
SIZE = (100, 100)


def main() -> int:
    if len(sys.argv) < 2 or not sys.argv[1].startswith("http"):
        return 1
    with urllib.request.urlopen(sys.argv[1], timeout=10) as r:
        data = r.read()
    img = Image.open(io.BytesIO(data)).convert("RGB")
    if img.size != SIZE:
        img = img.resize(SIZE, Image.LANCZOS)
    tmp = OUT + ".tmp"
    img.save(tmp, "BMP")  # RGB -> BMP 24 bit, BI_RGB (non compresso)
    os.replace(tmp, OUT)  # atomico: il dial non scarica mai un file a meta'
    return 0


if __name__ == "__main__":
    sys.exit(main())
