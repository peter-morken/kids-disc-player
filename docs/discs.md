# Making discs

Each disc's NFC tag holds a Spotify link. The player reads that link, so adding
a new disc never needs a firmware change.

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

The front of the disc has a 46 mm recess for a round picture sticker (album
art works well). Kids who can't read yet choose by the picture.

## Troubleshooting

| Serial log | Cause |
|---|---|
| `Disc has no readable NDEF URL/text record` | The tag is blank, or isn't an NTAG21x (MIFARE Classic isn't supported) |
| `Not a Spotify track/album/playlist link` | The link is a short link, an artist page, or not from Spotify |
| `Sonos AddURIToQueue failed: HTTP 500` | Spotify isn't linked in the Sonos app, or the item isn't available in your region |
