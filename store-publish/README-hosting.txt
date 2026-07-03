purplepois0n apt repo — serve this directory over HTTPS.

nginx example:
  location / {
    root /path/to/this/dir;
    autoindex off;
  }

Device sources.list.d entry:
  deb [trusted=yes] https://YOUR_HOST/ purplepois0n main
