# Mahjong tile artwork

tiles.svg contains the original tile artwork for cjong4-web as an SVG symbol
sprite. It includes the 34 standard tile faces, three red fives, and one tile
back.

The symbol IDs use common mahjong notation:

- tile-1m through tile-9m: characters
- tile-1p through tile-9p: circles
- tile-1s through tile-9s: bamboos
- tile-1z through tile-7z: east, south, west, north, white, green, red
- tile-0m, tile-0p, tile-0s: red fives
- tile-back: tile back

Use a symbol from HTML:

    <svg viewBox="0 0 72 100" role="img" aria-label="一萬">
      <use href="/assets/tiles.svg#tile-1m"></use>
    </svg>

Open tiles-preview.html through a local HTTP server to inspect the full set.

## License and provenance

The artwork was created specifically for cjong4-web without copying external
tile images. It is covered by the repository's MIT License. The sprite embeds
no third-party images, scripts, or web fonts.
