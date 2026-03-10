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

// Process index.html and inject environment variables
console.log('Injecting environment variables into index.html...');
let html = fs.readFileSync('index.html', 'utf8');

// These must match the names you set in Netlify dashboard
const tmdbKey = process.env.TMDB_API_KEY || 'TMDB_API_KEY_PLACEHOLDER';
const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL || 'SUPABASE_URL_PLACEHOLDER';
const supabaseAnonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY || 'SUPABASE_ANON_KEY_PLACEHOLDER';

html = html.split('TMDB_API_KEY_PLACEHOLDER').join(tmdbKey);
html = html.split('SUPABASE_URL_PLACEHOLDER').join(supabaseUrl);
html = html.split('SUPABASE_ANON_KEY_PLACEHOLDER').join(supabaseAnonKey);

fs.writeFileSync(path.join(dist, 'index.html'), html);

console.log('Build complete! Publish directory should be: dist');
