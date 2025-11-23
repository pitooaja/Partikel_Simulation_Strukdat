# Penjelasan Detail Kode Particle Simulation

## 1. Kode Versi Sederhana (main.cpp saat ini)

### Include dan Setup
```cpp
#include <SFML/Graphics.hpp>
```
- **Penjelasan**: Mengimpor library SFML untuk rendering grafis, window management, dan event handling
- **Fungsi**: Memberikan akses ke semua kelas SFML seperti `RenderWindow`, `CircleShape`, `Vector2f`, dll

### Inisialisasi Window
```cpp
sf::RenderWindow window(sf::VideoMode({800, 600}), "Bouncing Circle - Basic");
window.setFramerateLimit(60);
```
- **Penjelasan**: 
  - Membuat window dengan resolusi 800x600 pixel
  - `setFramerateLimit(60)`: Membatasi FPS ke 60 untuk konsistensi performa
- **Fungsi**: Window adalah kanvas utama untuk menampilkan semua grafis

### Variabel State Bola
```cpp
float radius = 20.f;
sf::Vector2f position(200.f, 150.f);
sf::Vector2f velocity(220.f, 160.f);
sf::Clock clock;
```
- **radius**: Ukuran jari-jari bola (20 pixel)
- **position**: Posisi awal bola di koordinat (200, 150)
- **velocity**: Kecepatan per detik dalam arah X (220 px/s) dan Y (160 px/s)
- **clock**: Timer untuk menghitung delta time (waktu antara frame)

### Main Loop - Event Handling
```cpp
while (window.isOpen()) {
    while (auto event = window.pollEvent()) {
        if (!event) continue;
        if (auto* closeEvent = event->getIf<sf::Event::Closed>()) {
            window.close();
        }
    }
```
- **Penjelasan**:
  - `window.isOpen()`: Loop berjalan selama window masih terbuka
  - `pollEvent()`: Mengambil event dari queue (SFML 3.0 style dengan pointer)
  - `getIf<sf::Event::Closed>()`: Cek jika event adalah tombol close window
  - Jika ya, tutup window dan keluar dari loop

### Delta Time Calculation
```cpp
const float dt = clock.restart().asSeconds();
const auto size = window.getSize();
const float w = static_cast<float>(size.x);
const float h = static_cast<float>(size.y);
```
- **dt (delta time)**: 
  - `clock.restart()`: Reset timer dan return waktu sejak restart terakhir
  - `.asSeconds()`: Konversi ke detik (biasanya 0.016 detik untuk 60 FPS)
  - **Penting**: Digunakan untuk frame-independent movement (kecepatan konsisten di semua FPS)
- **w, h**: Lebar dan tinggi window untuk collision detection

### Update Posisi (Physics)
```cpp
position += velocity * dt;
```
- **Penjelasan**: 
  - Menambahkan kecepatan dikali waktu ke posisi
  - Contoh: velocity (220, 160) * dt (0.016) = perpindahan (3.52, 2.56) pixel per frame
  - **Frame-independent**: Bola bergerak dengan kecepatan yang sama di 30 FPS atau 60 FPS

### Collision Detection - Dinding Horizontal
```cpp
if (position.x - radius <= 0.f) {
    position.x = radius;
    velocity.x = -velocity.x;
} else if (position.x + radius >= w) {
    position.x = w - radius;
    velocity.x = -velocity.x;
}
```
- **Penjelasan**:
  - **Kiri**: Jika posisi X - radius <= 0, bola menyentuh dinding kiri
    - `position.x = radius`: Pindahkan bola ke dalam agar tidak keluar batas
    - `velocity.x = -velocity.x`: Balik arah kecepatan horizontal (bounce)
  - **Kanan**: Jika posisi X + radius >= lebar window, bola menyentuh dinding kanan
    - Logika sama, tapi untuk sisi kanan

### Collision Detection - Dinding Vertikal
```cpp
if (position.y - radius <= 0.f) {
    position.y = radius;
    velocity.y = -velocity.y;
} else if (position.y + radius >= h) {
    position.y = h - radius;
    velocity.y = -velocity.y;
}
```
- **Penjelasan**: Sama seperti horizontal, tapi untuk dinding atas dan bawah
- **Atas**: position.y - radius <= 0
- **Bawah**: position.y + radius >= tinggi window

### Rendering
```cpp
window.clear(sf::Color(20, 20, 40));
sf::CircleShape circle(radius);
circle.setOrigin({radius, radius});
circle.setPosition(position);
circle.setFillColor(sf::Color(100, 200, 255));
window.draw(circle);
window.display();
```
- **clear()**: Bersihkan frame sebelumnya dengan warna gelap (RGB: 20, 20, 40)
- **CircleShape**: Buat bentuk lingkaran dengan radius tertentu
- **setOrigin()**: Set titik pusat lingkaran (untuk rotasi/posisi yang akurat)
- **setPosition()**: Posisikan lingkaran di koordinat tertentu
- **setFillColor()**: Warna biru muda (RGB: 100, 200, 255)
- **draw()**: Tambahkan ke render queue
- **display()**: Tampilkan semua yang sudah di-draw ke layar

---

## 2. Konsep Algoritma yang Sudah Diimplementasikan

### A. Brute Force Collision Detection

**Konsep**:
- Mengecek **semua pasangan** bola untuk collision
- Kompleksitas: **O(n²)** dimana n = jumlah bola

**Algoritma**:
```
Untuk setiap bola i:
    Untuk setiap bola j (dimana j > i):
        Hitung jarak antara bola i dan j
        Jika jarak < (radius_i + radius_j):
            Tentukan normal vector (arah collision)
            Pisahkan posisi (resolve overlap)
            Hitung dan terapkan pantulan kecepatan
```

**Kelebihan**:
- ✅ Implementasi sederhana
- ✅ Mudah dipahami
- ✅ Cocok untuk jumlah bola sedikit (< 50)

**Kekurangan**:
- ❌ Lambat untuk banyak bola (100+ bola = 10,000+ pengecekan)
- ❌ Tidak efisien secara komputasi

**Kode Pseudo**:
```cpp
void checkCollisionBruteForce(balls) {
    for (i = 0; i < balls.size(); i++) {
        for (j = i+1; j < balls.size(); j++) {
            distance = sqrt((balls[j].x - balls[i].x)² + (balls[j].y - balls[i].y)²)
            if (distance < balls[i].radius + balls[j].radius) {
                // Handle collision
            }
        }
    }
}
```

---

### B. Quadtree Collision Detection

**Konsep**:
- **Spatial Partitioning**: Membagi area menjadi 4 kuadran secara rekursif
- Hanya mengecek collision antar bola yang berada di **kuadran yang sama**
- Kompleksitas: **O(n log n)** rata-rata, **O(n²)** worst case

**Struktur Data**:
```
Quadtree
├── Bounds (AABB: x, y, width, height)
├── Objects (array bola di node ini)
├── Level (kedalaman tree)
└── Nodes[4] (4 child nodes: NW, NE, SW, SE)
```

**Algoritma Insert**:
```
1. Cek apakah bola berada dalam bounds node ini
2. Jika tidak, return (tidak masuk node ini)
3. Jika objects < MAX_OBJECTS atau level >= MAX_LEVELS:
   - Tambahkan bola ke objects array
4. Jika belum di-subdivide:
   - Bagi node menjadi 4 kuadran
5. Tentukan kuadran mana bola masuk
6. Recursively insert ke child node yang sesuai
```

**Algoritma Retrieve (Query)**:
```
1. Tentukan kuadran mana bola target berada
2. Recursively retrieve dari child node yang sesuai
3. Tambahkan semua objects di node ini ke hasil
4. Return semua bola yang berpotensi collision
```

**Kelebihan**:
- ✅ Sangat efisien untuk banyak bola (1000+ bola)
- ✅ Mengurangi pengecekan yang tidak perlu
- ✅ Skalabel untuk area besar

**Kekurangan**:
- ❌ Implementasi lebih kompleks
- ❌ Overhead memory untuk tree structure
- ❌ Perlu rebuild tree setiap frame (atau update incremental)

**Visualisasi**:
```
Window (800x600)
├── NW (0-400, 0-300)
│   ├── NW (0-200, 0-150)
│   ├── NE (200-400, 0-150)
│   ├── SW (0-200, 150-300)
│   └── SE (200-400, 150-300)
├── NE (400-800, 0-300)
├── SW (0-400, 300-600)
└── SE (400-800, 300-600)
```

---

### C. Collision Response (Physics)

**Konsep**: Setelah detect collision, perlu **resolve** (memisahkan) dan **respond** (memantulkan)

**1. Resolve Overlap**:
```cpp
overlap = (radius1 + radius2) - distance
normal = (pos2 - pos1) / distance  // Normal vector
pos1 -= normal * (overlap * 0.5)    // Pindahkan bola 1
pos2 += normal * (overlap * 0.5)    // Pindahkan bola 2
```
- Memisahkan bola yang overlap agar tidak saling menembus

**2. Bounce Velocity**:
```cpp
// Hitung komponen kecepatan sepanjang normal
dot1 = velocity1 · normal
dot2 = velocity2 · normal

// Reflect velocity
velocity1 -= 2 * dot1 * normal
velocity2 -= 2 * dot2 * normal
```
- **Dot product**: Menghitung seberapa banyak kecepatan searah normal
- **Reflection**: Membalik komponen kecepatan sepanjang normal (hukum pantulan)

**Formula Lengkap** (dengan massa, jika ada):
```
v1' = v1 - 2m2/(m1+m2) * (v1-v2)·n * n
v2' = v2 - 2m1/(m1+m2) * (v2-v1)·n * n
```
- Untuk massa sama: m1 = m2, jadi koefisien = 1

---

### D. Trail Effect System

**Konsep**: Menyimpan history posisi untuk membuat efek jejak/trail

**Data Structure**:
```cpp
struct Ball {
    std::vector<sf::Vector2f> trail;  // Array posisi sebelumnya
}
```

**Update Trail**:
```cpp
if (speed > threshold) {
    trail.push_back(current_position)
    if (trail.size() > MAX_TRAIL) {
        trail.erase(trail.begin())  // Hapus yang paling tua
    }
}
```
- Hanya simpan trail jika bola bergerak cepat
- Batasi jumlah trail (misal 35 posisi)

**Render Trail**:
```cpp
for (each position in trail) {
    t = index / trail.size()  // Normalized position (0-1)
    alpha = t² * maxAlpha     // Fade out (kuadratik)
    size = minSize + t * (maxSize - minSize)
    
    // Multi-layer untuk efek blur
    for (layer = 0; layer < 3; layer++) {
        Draw circle dengan alpha dan size yang berbeda
    }
}
```
- **Fade out**: Trail di belakang lebih transparan
- **Size gradient**: Trail di belakang lebih kecil
- **Multi-layer**: 3 layer dengan ukuran berbeda untuk efek blur

---

### E. Glow Effect

**Konsep**: Menambahkan lingkaran transparan di sekitar bola untuk efek cahaya

**Implementation**:
```cpp
// Outer glow (besar, sangat transparan)
outerGlow = Circle(radius * 1.8, alpha=60)
// Inner glow (sedang, lebih terang)
innerGlow = Circle(radius * 1.4, alpha=120)
// Bola utama
mainBall = Circle(radius, alpha=255)
```

**Render Order** (dari belakang ke depan):
1. Trail (paling belakang)
2. Outer glow
3. Inner glow
4. Bola utama (paling depan)

---

## 3. Perbandingan Algoritma

### Brute Force vs Quadtree

| Aspek | Brute Force | Quadtree |
|-------|-------------|----------|
| **Kompleksitas** | O(n²) | O(n log n) avg |
| **Implementasi** | Sederhana | Kompleks |
| **Memory** | O(1) | O(n) |
| **Cocok untuk** | < 50 bola | > 50 bola |
| **Worst case** | O(n²) | O(n²) jika semua bola di 1 node |

### Kapan Pakai Apa?

**Brute Force**:
- Jumlah bola sedikit (< 50)
- Implementasi cepat
- Tidak perlu optimasi

**Quadtree**:
- Banyak bola (100+)
- Perlu performa tinggi
- Area besar dengan distribusi sparse

---

## 4. Optimasi Tambahan yang Bisa Diterapkan

1. **Spatial Hashing**: Alternatif Quadtree, lebih cepat untuk update
2. **Broad Phase / Narrow Phase**: 2 tahap collision detection
3. **Sweep and Prune**: Sort berdasarkan axis, cek overlap
4. **BVH (Bounding Volume Hierarchy)**: Tree structure yang lebih efisien
5. **GPU Collision Detection**: Gunakan shader untuk parallel processing

---

## 5. Struktur Data Penting

### AABB (Axis-Aligned Bounding Box)
```cpp
struct AABB {
    float x, y;        // Posisi kiri atas
    float width, height; // Ukuran
}
```
- **Fungsi**: Representasi area persegi untuk spatial partitioning
- **Contains**: Cek apakah bola berada dalam AABB
- **Intersects**: Cek apakah 2 AABB overlap

### Ball Structure (Extended)
```cpp
struct Ball {
    sf::Vector2f pos;      // Posisi (x, y)
    sf::Vector2f vel;      // Kecepatan (vx, vy)
    float size;            // Radius
    sf::Color color;       // Warna
    std::vector<Vector2f> trail; // History posisi
}
```

---

## 6. Tips Implementasi

1. **Delta Time**: Selalu pakai untuk frame-independent movement
2. **Collision Response**: Pisahkan dulu (resolve), baru pantulkan (respond)
3. **Trail**: Batasi jumlah untuk performa
4. **Quadtree**: Rebuild setiap frame atau update incremental
5. **Visualisasi**: Tampilkan bounds untuk debug Quadtree

---

## Kesimpulan

Kode ini mengimplementasikan:
- ✅ Basic physics (movement, collision)
- ✅ Brute Force collision detection
- ✅ Quadtree spatial partitioning
- ✅ Visual effects (trail, glow)
- ✅ Frame-independent animation

Semua konsep ini penting untuk membuat simulasi partikel yang realistis dan performant!

