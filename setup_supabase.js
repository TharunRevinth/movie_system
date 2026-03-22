const { createClient } = require('@supabase/supabase-js');

const supabaseUrl = 'https://xwzbcexvpvtprytqaggx.supabase.co';
const supabaseAnonKey = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Inh3emJjZXh2cHZ0cHJ5dHFhZ2d4Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzMwNjc3OTMsImV4cCI6MjA4ODY0Mzc5M30.4BhB86darviKf9r9xpMH937VxYUqxVFDNZS8jE7iEs8';

const supabase = createClient(supabaseUrl, supabaseAnonKey);

async function runSetup() {
    console.log("Checking Supabase connection...");
    
    // We cannot run raw SQL via the anon key for security reasons.
    // However, I will attempt to perform a test insert to see if the table exists.
    
    const { error } = await supabase.from('shows').select('count').limit(1);
    
    if (error && error.code === 'PGRST116') {
        console.error("❌ TABLE MISSING: The 'shows' table does not exist.");
    } else if (error) {
        console.error("❌ CONNECTION ERROR:", error.message);
    } else {
        console.log("✅ Tables are accessible!");
    }
}

console.log("--- Supabase Automatic Setup Attempt ---");
console.log("NOTE: Raw SQL injection is blocked by Supabase for security.");
console.log("Please run the provided SQL in your Supabase SQL Editor.");
runSetup();
