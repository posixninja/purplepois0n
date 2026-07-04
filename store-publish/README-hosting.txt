purplepois0n apt repo — serve this directory over HTTPS.

Publish directory: store-publish

1. Upload entire directory to static HTTPS host:

   rsync -avz "store-publish/" user@server:/var/www/purplepois0n-repo/

2. nginx location block:

   location /purplepois0n-repo/ {
     alias /var/www/purplepois0n-repo/;
     autoindex off;
   }

3. Device apt source (/var/jb/etc/apt/sources.list.d/purplepois0n.list):

   deb [trusted=yes] https://YOUR_HOST/purplepois0n-repo/purplepois0n main

4. On device:

   apt update
   apt install purplepois0n-smoke purplepois0n-zebra purplepois0n-sources

5. Dev SSH file mirror (alternative to HTTPS):

   ./build/bin/purplepois0n store sync --store-sync-mode file --normal-ssh -d YOUR_UDID

Set PURPLEPOIS0N_REPO_URL before seed-store.sh to bake URL into purplepois0n-sources.
