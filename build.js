const fs = require('fs');
const path = require('path');

// Ensure dist folder exists
const dist = path.join(__dirname, 'dist');
if (!fs.existsSync(dist)) {
    fs.mkdirSync(dist);
}

// Files to copy to the final deployment folder (dist)
const filesToCopy = [
    'index.js',
    'index.wasm',
    'bookings.dat',
    'shows.dat',
    'README.md',
    'shell_template.html'
];

filesToCopy.forEach(file => {
    if (fs.existsSync(file)) {
        fs.copyFileSync(file, path.join(dist, file));
        console.log(`Copied ${file} to dist/`);
    } else {
        console.warn(`Warning: ${file} not found, skipping copy.`);
    }
});

// Process template.html and inject environment variables into the final index.html
console.log('--- Environment Variable Check ---');
const keys = ['TMDB_API_KEY', 'NEXT_PUBLIC_SUPABASE_URL', 'NEXT_PUBLIC_SUPABASE_ANON_KEY'];
keys.forEach(k => {
    const val = process.env[k];
    console.log(`${k}: ${val ? 'FOUND (Length: ' + val.length + ')' : 'NOT FOUND'}`);
});

console.log('Injecting environment variables from template.html into dist/index.html...');
let html = fs.readFileSync('template.html', 'utf8');

const tmdbKey = process.env.TMDB_API_KEY || 'TMDB_API_KEY_PLACEHOLDER';
const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL || 'SUPABASE_URL_PLACEHOLDER';
const supabaseAnonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY || 'SUPABASE_ANON_KEY_PLACEHOLDER';

// Log if we are using placeholders
if (tmdbKey === 'TMDB_API_KEY_PLACEHOLDER') console.warn('WARNING: Using placeholder for TMDB_API_KEY');

html = html.replace(/TMDB_API_KEY_PLACEHOLDER/g, tmdbKey);
html = html.replace(/SUPABASE_URL_PLACEHOLDER/g, supabaseUrl);
html = html.replace(/SUPABASE_ANON_KEY_PLACEHOLDER/g, supabaseAnonKey);

fs.writeFileSync(path.join(dist, 'index.html'), html);

console.log('Build complete! Publish directory should be: dist');
