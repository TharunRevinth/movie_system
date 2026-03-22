const { createClient } = require('@supabase/supabase-js');
const fs = require('fs');
require('dotenv').config();

const supabase = createClient(process.env.NEXT_PUBLIC_SUPABASE_URL, process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY);
const mode = process.argv[2];

const SHOW_SIZE = 592; 
const BOOKING_SIZE = 348; 

async function runSync() {
    try {
        if (mode === 'pull') {
            console.log("[Sync] Pulling live cloud data...");
            const { data: bookings } = await supabase.from('bookings').select('*');
            
            // Format for C program
            const bookBuf = Buffer.alloc(4 + (500 * BOOKING_SIZE));
            bookBuf.writeInt32LE(bookings?.length || 0, 0);
            bookings?.forEach((b, i) => {
                const offset = 4 + (i * BOOKING_SIZE);
                bookBuf.write(b.id_short || "", offset, 15);
                bookBuf.write(b.customer_name || "", offset + 16, 63);
                bookBuf.writeInt32LE(b.show_id, offset + 80);
                bookBuf.writeInt32LE(b.num_seats || 0, offset + 84);
                
                // Parse '0,1-0,2' back into C arrays
                if (b.seats) {
                    const coords = b.seats.split('-');
                    coords.forEach((c, ci) => {
                        const [r, col] = c.split(',').map(Number);
                        bookBuf.writeInt32LE(r, offset + 88 + (ci * 4));
                        bookBuf.writeInt32LE(col, offset + 168 + (ci * 4));
                    });
                }
                
                bookBuf.writeFloatLE(b.total_amount || 0, offset + 248);
                bookBuf.write(b.status || "confirmed", offset + 316, 15);
                bookBuf.write(b.show_date || "", offset + 332, 15);
            });
            fs.writeFileSync('bookings_v4.dat', bookBuf);
            console.log("[Sync] Local database updated with Cloud seats.");
        }
        
        if (mode === 'push') {
            console.log("[Sync] Pushing terminal seats to Cloud...");
            const data = fs.readFileSync('bookings_v4.dat');
            const count = data.readInt32LE(0);
            const localBookings = [];

            for (let i = 0; i < count; i++) {
                const offset = 4 + (i * BOOKING_SIZE);
                const numSeats = data.readInt32LE(offset + 84);
                
                // Convert C arrays back to Web string format '0,1-0,2'
                let seatString = "";
                for(let s=0; s < numSeats; s++) {
                    const r = data.readInt32LE(offset + 88 + (s * 4));
                    const c = data.readInt32LE(offset + 168 + (s * 4));
                    seatString += `${r},${c}${s === numSeats-1 ? '' : '-'}`;
                }

                localBookings.push({
                    id_short: data.toString('utf8', offset, offset + 15).replace(/\0/g, '').trim(),
                    customer_name: data.toString('utf8', offset + 16, offset + 79).replace(/\0/g, '').trim(),
                    show_id: data.readInt32LE(offset + 80),
                    num_seats: numSeats,
                    seats: seatString || 'Terminal', // This makes it visible on the Web Grid!
                    total_amount: data.readFloatLE(offset + 248),
                    status: data.toString('utf8', offset + 316, offset + 331).replace(/\0/g, '').trim() || 'confirmed',
                    show_date: data.toString('utf8', offset + 332, offset + 347).replace(/\0/g, '').trim() || new Date().toISOString().split('T')[0]
                });
            }

            if (localBookings.length > 0) {
                const { error } = await supabase.from('bookings').upsert(localBookings, { onConflict: 'id_short' });
                if (error) throw error;
                console.log(`[Sync] Pushed ${localBookings.length} bookings. Seats are now LOCKED on web.`);
            }
        }
    } catch (err) { console.error("[Sync Error]", err.message); }
}
runSync();
