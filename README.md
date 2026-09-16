# Android SA-MP Map Editor (Native C++ / OpenGL ES 3.0)

Aplikasi Android mandiri (*standalone*) untuk melakukan 3D mapping GTA San Andreas / SA-MP (San Andreas Multiplayer) langsung dari HP Android **tanpa memerlukan PC dan tanpa harus menjalankan server SA-MP terlebih dahulu**.

---

## Fitur Utama

- **100% Standalone**: Tidak butuh koneksi server, tidak butuh client GTA SA aktif, dan tidak butuh PC.
- **Native RenderWare Engine**: Mem-parse format biner asli RenderWare GTA SA (`.img`, `.dff`, `.txd`, `.ide`, `.ipl`) langsung via C++20 dan OpenGL ES 3.0.
- **Bundled SA-MP 0.3.7 Assets**: Sudah dilengkapi seluruh katalog objek khas SA-MP (1433+ model dari `SAMP.ide` dan `SAMP.img` seperti ramp stunt, gerbang, furnitur, tabung neon, interior) langsung di dalam APK.
- **Dukungan Mounting `gta3.img`**: User dapat mengimpor file `gta3.img` dari storage HP menggunakan Android Storage Access Framework (SAF) untuk memuat model dan map original San Andreas.
- **Presisi 100% Bit-Exact SA-MP**: Koordinat float IEEE 754 dan rotasi Euler derajat GTA SA ($Z \to X \to Y$) yang sinkron bit-exact dengan format server SA-MP.
- **Sistem Anti-OOM (Out Of Memory)**: Partisi spasial SpatialGrid2D ($250\text{m} \times 250\text{m}$) dengan distance culling dinamis dan VRAM cache agar RAM HP tetap dingin (< 250 MB).
- **3D Transform Gizmo & Touch Raycast**: Sentuh layar untuk memilih objek, geser panah 3D (Merah = X, Hijau = Y, Biru = Z) untuk memindahkan objek, duplikasi, atau hapus.
- **Direct PAWN Script Exporter**: Ekspor hasil mapping ke format script `CreateDynamicObject(...)` (Streamer plugin) atau `CreateObject(...)` dengan satu ketukan tombol.

---

## Cara Build APK Menggunakan GitHub Actions (GHA)

Tidak perlu spesifikasi laptop yang berat, build APK dapat dilakukan 100% gratis di cloud melalui GitHub Actions:

### 1. Inisialisasi Git dan Push ke GitHub
Buka terminal di folder proyek ini (`android-samp-map-editor`):
```bash
git init
git add .
git commit -m "Initial commit SA-MP Android Map Editor"
git branch -M main
git remote add origin https://github.com/USERNAME_KAMU/NAMA_REPO_KAMU.git
git push -u origin main
```

### 2. Unduh APK dari GitHub Actions
1. Buka repository kamu di browser GitHub.
2. Klik tab **Actions**.
3. Workflow **"Build SA-MP Map Editor APK"** akan otomatis berjalan saat push (atau bisa ditekan tombol **Run workflow** secara manual).
4. Setelah selesai (centang hijau), klik workflow run tersebut dan gulir ke bagian **Artifacts**.
5. Unduh file zip **`SAMP-Map-Editor-Debug-APK`**, ekstrak, dan instal file `.apk` ke ponsel Android kamu!

---

## Cara Menggunakan Aplikasi di Android

1. **Buka Aplikasi**: Tampilan awal akan menyajikan scene 3D dengan koordinat grid.
2. **Tambah Objek**: Tekan tombol **`+ Object`** berwarna oranye di kanan bawah. Ketik nama objek atau ID (misalnya `19800` atau `neon`), lalu sentuh untuk memunculkan objek di depan kamera.
3. **Pindahkan Objek**: Sentuh objek di layar. Panah 3D Transform Gizmo (Merah, Hijau, Biru) akan muncul di atas objek. Geser panah untuk memindahkan objek ke koordinat yang diinginkan.
4. **Navigasi Kamera**:
   - Usap 1 jari pada area kosong untuk memutar arah pandang (Yaw & Pitch).
   - Cubit 2 jari (Pinch) untuk memperbesar / zoom kamera.
   - Gunakan tombol kontrol D-Pad di kiri bawah (Fwd, Back, Left, Right, Up, Down) untuk terbang bebas.
5. **Mount Map Asli GTA SA (Opsional)**:
   - Tekan tombol **`Mount gta3.img`** di bilah atas.
   - Pilih file `gta3.img` dari penyimpanan HP kamu untuk memuat map San Andreas.
6. **Ekspor Mappingan**:
   - Tekan tombol **`Export .PWN`** di bilah atas.
   - Dialog akan menampilkan kode PAWN yang sudah terformat rapi.
   - Tekan **Salin ke Clipboard**, lalu paste langsung ke gamemode server SA-MP kamu!
