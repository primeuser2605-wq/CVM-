# CVM++ portfolio site — deployment guide

This is a static portfolio site for the CVM++ project. Four files, no build step, ready for GitHub Pages.

## Files

```
index.html    main page (HTML structure)
styles.css    styling
cvm.js        JavaScript port of the CVM++ pipeline (lexer + parser + compiler + VM)
main.js       UI wiring for the live demo
```

Total ~63 KB, no external dependencies except Google Fonts (loaded over CDN).

---

## Option 1 — Deploy to the same repo as the project (`/docs` folder)

The simplest path: put the site inside a `docs/` folder of your existing `CVM-` repo. GitHub Pages can serve directly from there.

In your project repo, on Windows MSYS2 terminal:

```bash
cd ~/CVM-                                    # your existing CVM++ repo
mkdir -p docs
# Copy the 4 site files into docs/
cp /path/to/CVMpp_site/index.html  docs/
cp /path/to/CVMpp_site/styles.css  docs/
cp /path/to/CVMpp_site/cvm.js      docs/
cp /path/to/CVMpp_site/main.js     docs/

git add docs/
git commit -m "Add portfolio site"
git push
```

Then on GitHub:

1. Go to your repo → **Settings** → **Pages** (left sidebar)
2. Under "Build and deployment", set **Source** to "Deploy from a branch"
3. **Branch:** `main`, **Folder:** `/docs`
4. Click **Save**

Wait 1-2 minutes. Your site will be live at:
**https://primeuser2605-wq.github.io/CVM-/**

---

## Option 2 — Dedicated repo (e.g. `cvm-site`)

Cleaner separation. Create a new repo for just the site.

```bash
# Make a new repo on github.com first (e.g. "cvm-site"), then:
mkdir cvm-site && cd cvm-site
# Copy all 4 files into here
cp /path/to/CVMpp_site/*.html .
cp /path/to/CVMpp_site/*.css .
cp /path/to/CVMpp_site/*.js .

git init
git branch -M main
git remote add origin https://github.com/primeuser2605-wq/cvm-site.git
git add .
git commit -m "Initial site"
git push -u origin main
```

Then in repo Settings → Pages, set Source = `main` branch, folder = `/ (root)`.

Site goes live at **https://primeuser2605-wq.github.io/cvm-site/**

---

## Option 3 — User site at the root (`primeuser2605-wq.github.io`)

If you want the site at the bare domain `https://primeuser2605-wq.github.io` (no subpath), the repo must be named exactly `primeuser2605-wq.github.io`. Same steps as Option 2, just with that special repo name.

---

## Test it locally first

Before pushing, you can preview the site in a browser:

```bash
cd /path/to/CVMpp_site
python -m http.server 8000
```

Then open **http://localhost:8000** in your browser. Try the live demo — if it works locally, it'll work on Pages.

---

## What to expect

- **Hero** with the pipeline diagram and key stats
- **Five-stage breakdown** with code for each transformation
- **ISA table** (17 opcodes)
- **`.cvmb` file format** spec
- **Live in-browser demo** — visitors type CVM++ code and watch lexer/parser/compiler/VM all update in real time
- **Project file tree**
- Footer with GitHub links

The live demo is the killer feature. The JavaScript pipeline mirrors the C++ implementation closely, so what visitors see in the browser is faithful to how the real compiler/VM behaves.

---

## During an interview

URL to share: whichever Pages URL above you set up.

Two-minute walkthrough:

1. Point to the pipeline diagram and the stats strip — "five stages, 17 opcodes, written in C++17 with zero external dependencies"
2. Scroll to the live demo. Click "fibonacci" or "factorial". The interviewer sees source on the left, then tokens, AST, bytecode, and VM output update simultaneously.
3. Type a small change in the source — they see all four output panels update live. This makes the pipeline concrete.
4. Click "GitHub" to show the real C++ implementation.

---

## Troubleshooting

**Fonts look wrong.**
Google Fonts is blocked or slow. The site falls back to system fonts (Georgia / system-ui / Menlo). Still readable.

**Demo doesn't run.**
Check the browser console (F12). The `cvm.js` should attach `CVM` to `window`. If you see "CVM is undefined," scripts loaded out of order — make sure `cvm.js` is included before `main.js`.

**404 on GitHub Pages.**
Pages takes 1-2 minutes to deploy after the first push. Refresh after a few minutes. Also confirm the Source branch and folder are set correctly in repo Settings → Pages.
