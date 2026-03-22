const { createClient } = require('@supabase/supabase-js');
const fs = require('fs');
require('dotenv').config();

const supabase = createClient(process.env.NEXT_PUBLIC_SUPABASE_URL, process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY);
const mode = process.argv[2];

// FIXED: Exact byte sizes matching main.c structs
const SHOW_SIZE = 592; 
const BOOKING_SIZE = 348; 

function safeWriteInt32(buf, val, offset) {
    const wrapped = Number(BigInt.asIntN(32, BigInt(val || 0)));
    buf.writeInt32LE(wrapped, offset);
}

async function runSync() {
    try {
        if (mode === 'pull') {
            console.log("[Sync] Pulling live data...");
            const { data: shows } = await supabase.from('shows').select('*');
            const { data: bookings } = await supabase.from('bookings').select('*');

            // Write shows_v4.dat
            const showBuf = Buffer.alloc(4 + (200 * SHOW_SIZE));
            showBuf.writeInt32LE(shows?.length || 0, 0);
            shows?.forEach((s, i) => {
                const offset = 4 + (i * SHOW_SIZE);
                safeWriteInt32(showBuf, s.id, offset);
                showBuf.write(s.name || "", offset + 4, 63);
                showBuf.write(s.timing || "", offset + 68, 31);
                showBuf.writeFloatLE(s.price || 0, offset + 100);
                showBuf.write(s.img || "", offset + 104, 255);
                showBuf.write(s.region || "", offset + 360, 31);
            });
            fs.writeFileSync('shows_v4.dat', showBuf);

            // Write bookings_v4.dat
            const bookBuf = Buffer.alloc(4 + (500 * BOOKING_SIZE));
            bookBuf.writeInt32LE(bookings?.length || 0, 0);
            bookings?.forEach((b, i) => {
                const offset = 4 + (i * BOOKING_SIZE);
                bookBuf.write(b.id_short || "", offset, 15);
                bookBuf.write(b.customer_name || "", offset + 16, 63);
                safeWriteInt32(bookBuf, b.show_id, offset + 80);
                bookBuf.writeInt32LE(b.num_seats || 0, offset + 84);
                bookBuf.writeFloatLE(b.total_amount || 0, offset + 248);
                bookBuf.write(b.user_email || "", offset + 252, 63);
                bookBuf.write(b.status || "confirmed", offset + 316, 15);
                bookBuf.write(b.show_date || "", offset + 332, 15);
            });
            fs.writeFileSync('bookings_v4.dat', bookBuf);
            console.log(`[Sync] Local files updated successfully.`);
        }
        
        if (mode === 'push') {
            console.log("[Sync] Pushing terminal bookings to Cloud...");
            if (!fs.existsSync('bookings_v4.dat')) return;
            const data = fs.readFileSync('bookings_v4.dat');
            const count = data.readInt32LE(0);
            const localBookings = [];

            for (let i = 0; i < count; i++) {
                const offset = 4 + (i * BOOKING_SIZE);
                let rawDate = data.toString('utf8', offset + 332, offset + 347).replace(/\0/g, '').trim();
                if (!rawDate) rawDate = new Date().toISOString().split('T')[0];

                localBookings.push({
                    id_short: data.toString('utf8', offset, offset + 15).replace(/\0/g, '').trim(),
                    customer_name: data.toString('utf8', offset + 16, offset + 79).replace(/\0/g, '').trim(),
                    show_id: data.readInt32LE(offset + 80),
                    num_seats: data.readInt32LE(offset + 84),
                    total_amount: data.readFloatLE(offset + 248),
                    user_email: data.toString('utf8', offset + 252, offset + 315).replace(/\0/g, '').trim() || 'terminal@cinebooking.com',
                    status: data.toString('utf8', offset + 316, offset + 331).replace(/\0/g, '').trim() || 'confirmed',
                    show_date: rawDate,
                    seats: 'Terminal-Booking'
                });
            }

            if (localBookings.length > 0) {
                const { error } = await supabase.from('bookings').upsert(localBookings, { onConflict: 'id_short' });
                if (error) throw error;
                console.log(`[Sync] Pushed ${localBookings.length} bookings.`);
            }
        }
    } catch (err) {
        console.error("[Sync Error]", err.message);
    }
}
runSync();
