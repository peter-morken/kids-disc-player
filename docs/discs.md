# Making discs

Each disc's NFC tag holds a Spotify link. The player reads that link, so adding
a new disc never needs a firmware change.

A disc is printed from `cad/stl/disc.stl`: 60 mm across and 6 mm
thick. One face has a round recess, 50 mm across and 0.4 mm deep, for a
49 mm label. In the middle of that recess is a 26.5 mm pocket, 0.8 mm deep,
for the NFC sticker. The tag reads through the plastic, so the disc works
either way round in the slot.

You need, per disc:

- one NTAG215 sticker, at most 26.5 mm across (NTAG213 works too; MIFARE
  Classic doesn't);
- one Niimbot R4949 label (49 mm round), printed on a Niimbot B21.

## 1. Get the link

In Spotify, open the album, playlist or song, then tap **Share → Copy link**.
You get something like:

```
https://open.spotify.com/album/1ATL5GLyefJaxhQzSPVrLX?si=...
```

Supported: tracks, albums, playlists, podcast shows and episodes. Not
supported: artists (Sonos can't queue them), and `spotify.link/...` short
links. If Spotify gives you a short link, open it in a browser and copy the
`open.spotify.com` URL it redirects to.

An album or a playlist you made for the disc usually works better than a
single song: the music keeps going after the first track.

## 2. Write the tag

Use any NFC writer app. These steps are for **NFC Tools** (iOS and Android,
free):

1. **Write → Add a record → URL/URI**, then paste the link.
2. Tap **Write** and hold the phone against the disc's sticker.

`spotify:album:...` URIs written as a **Text** record also work.

Optional: once you're sure about the disc, lock it with **Other → Lock tag**.
Locking is **permanent**, but it stops anyone from overwriting the tag by accident.

## 3. Label it

Print the label on the Niimbot B21 with R4949 (49 mm round) labels, from the
Niimbot app:

- **One big picture, no text.** Album art, a photo, an animal: toddlers
  can't read, they choose by the picture. Fill the whole circle, and pick
  pictures that look different from each other at a glance.
- **PET or waterproof labels** survive sticky hands better than paper ones.

Then put the disc together:

1. Stick the NFC tag flat in the centre pocket.
2. Stick the label into the recess over it.

You can write the Spotify link to the tag (step 2) before or after labelling:
the phone reads through the label.

## Troubleshooting

| Serial log | Cause |
|---|---|
| `Disc has no readable NDEF URL/text record` | The tag is blank, or isn't an NTAG21x (MIFARE Classic isn't supported) |
| `Not a Spotify track/album/playlist link` | The link is a short link, an artist page, or not from Spotify |
| `Sonos AddURIToQueue failed: HTTP 500` | Spotify isn't linked in the Sonos app, or the item isn't available in your region |
