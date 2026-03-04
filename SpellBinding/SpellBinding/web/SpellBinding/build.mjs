import { build } from 'esbuild';

await build({
  entryPoints: ['src/app.jsx'],
  bundle: true,
  outfile: 'app.js',
  format: 'iife',
  target: ['es2020'],
  jsx: 'automatic',
  minify: true,
  sourcemap: false,
  legalComments: 'none'
});

await build({
  entryPoints: ['src/hud.jsx'],
  bundle: true,
  outfile: 'hud.js',
  format: 'iife',
  target: ['es2020'],
  jsx: 'automatic',
  minify: true,
  sourcemap: false,
  legalComments: 'none'
});
