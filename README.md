# 🎬 Movie Ticket Booking System (CineBooking)

A professional, menu-driven terminal application with a Web (WASM) interface.

## 🚀 Features
- **Pro Terminal UI:** ANSI-colored menu with screen clearing and interactive reports.
- **Web Interface:** Fully functional movie booking system running in the browser via WebAssembly.
- **Hybrid Storage:** Uses local `.dat` files for the terminal and **IDBFS (IndexedDB)** for browser persistence.
- **Smart Sync:** Seamlessly bridge terminal data to the web version using Git-based data seeding.

## 🛠️ Tech Stack
- **Language:** C (Standard IO, Structs, File Handling)
- **Web:** Emscripten (WASM), HTML5, CSS3, Vanilla JS
- **Version Control:** Git

## 🖥️ Usage (Terminal)
1. Compile: `gcc main.c -o movie_system`
2. Run: `./movie_system`

