# WorkoutLang

Antrenman programlarını tasarlamak, yapılandırmak ve belgelemek için geliştirilmiş bir alan-özgü dil (DSL). C dilinde yazılmıştır; CSE 341 — Programlama Dilleri Kavramları dersi kapsamında geliştirilmiştir.

---

## Genel Bakış

WorkoutLang, fitness koçlarının ve sporcuların — egzersizleri, setleri, tekrar sayılarını, dinlenme sürelerini ve ilerleme kurallarını — genel amaçlı bir programlama dili öğrenmek zorunda kalmadan okunabilir ve belirsizlikten uzak bir biçimde tanımlamasına olanak tanır.

```
workout UpperBody {
    day Monday {
        set benchpress reps 4 x 10 rest 90s
        set overhead_press reps 3 x 8 rest 60s
    }
}

routine myWeek {
    int totalSets = 5
    load UpperBody
    if totalSets > 3 {
        print "Heavy week"
    }
    repeat 4 weeks {
        load UpperBody
    }
}
```

---

## Dil Özellikleri

### Türler
| Tür | Açıklama | Örnek |
|---|---|---|
| `int` | 32-bit işaretli tam sayı | `3`, `10` |
| `float` | IEEE 754 çift duyarlıklı | `1.5`, `72.5` |
| `bool` | Mantıksal değer | `true`, `false` |
| `duration` | Birim ekiyle süre | `90s`, `2min`, `1hr` |
| `weight` | Birim ekiyle ağırlık | `80kg`, `135lb` |
| `Day` | Kayıt türü (setCount alanı) | `day` anahtar sözcüğüyle tanımlanır |

### Kontrol Yapıları
- `if / else` — koşullu yürütme; koşul `bool` türünde olmak zorunda
- `while` — koşul her iterasyonda yeniden değerlendirilir (W-True / W-False semantiği)
- `repeat N weeks` — sabit sayıda döngü, iterasyon sayacı kullanıcıya açık değil

### Alana Özgü Yapılar
- `workout / day / set` — yapılandırılmış antrenman tanımlama hiyerarşisi
- `load` — adlandırılmış bir antrenmandaki tüm günleri çalıştırır
- `print` — herhangi bir değeri stdout'a yazar

### Fonksiyonlar
- Tiplenmiş parametreler ve dönüş türleri
- Değerle çağırma (call-by-value) parametre aktarımı (Sebesta §9.2)
- Özyinelemeli (recursive) çağrılar desteklenmektedir

### Tür Sistemi
- **Güçlü tipleme** — her tür hatası yürütmeden önce tespit edilir
- **Örtük tür dönüşümü yok** — `int → float` genişlemesine bile izin verilmez
- **Ad eşdeğerliği** — `duration` ve `weight`, ikisi de sayısal değer saklasa da birbirinden ayrı türlerdir
- **Statik (sözcüksel) kapsam** — isim çözümleme, çağrı sırasına değil, kaynak kodun metinsel yapısına göre yapılır

---

## Derleme

```bash
git clone <repo-url>
cd workoutlang
make
```

`gcc` ve C11 desteği gereklidir. `-Wall -Wextra -std=c11` bayraklarıyla sıfır uyarıyla derlenir.

Temizleyip yeniden derlemek için:

```bash
make clean && make
```

---

## Kullanım

```bash
# Sözdizimi analizi + tür denetimi + yürütme (varsayılan)
./workoutlang <dosya.wl>

# Sözdizimi analizi yapıp AST yazdır
./workoutlang --dump-ast <dosya.wl>

# Sözdizimi analizi + yalnızca tür denetimi (yürütme yok)
./workoutlang --type-check <dosya.wl>

# Yalnızca token akışını yazdır
./workoutlang --lex <dosya.wl>
```

---

## Örnek Çıktı

`./workoutlang tests/valid1.wl` çalıştırıldığında:

```
[UpperBody — Monday]
  set benchpress: 4 x 10, rest 90s
  set overhead_press: 3 x 8, rest 1min
  set tricep_dip: 3 x 12, rest 45s
[UpperBody — Wednesday]
  set pullup: 4 x 8, rest 90s
  set row: 3 x 10, rest 1min
Heavy week
[UpperBody — Monday]
  set benchpress: 4 x 10, rest 90s
  ...
```

---

## Test Programları

```bash
# Geçerli programlar — başarıyla ayrıştırılır, tür denetiminden geçer ve yürütülür
./workoutlang tests/valid1.wl   # workout + day + set + routine + if/else + repeat
./workoutlang tests/valid2.wl   # func + return + while + func_call
./workoutlang tests/valid3.wl   # bool döndüren func + && operatörü + tekli !

# Ayrıştırma hataları — satır numarasıyla reddedilir
./workoutlang tests/error1.wl   # eksik kapanış parantezi
./workoutlang tests/error2.wl   # set ile reps arasında 'x' eksik
./workoutlang tests/error3.wl   # bilinmeyen karakter '@'
./workoutlang tests/error4.wl   # set ifadesinde rest süresi eksik
./workoutlang tests/error5.wl   # üst düzeyde deyim

# Tür / çalışma zamanı hataları
./workoutlang tests/type_error1.wl   # bool değişkene int atama   → [line 3] Type error
./workoutlang tests/type_error2.wl   # sıfıra bölme               → [line 2] Runtime error
./workoutlang tests/type_error3.wl   # if koşulu bool değil       → [line 3] Type error
```

---

## Proje Yapısı

```
workoutlang/
├── Makefile
├── README.md
├── README_TR.md
├── src/
│   ├── main.c              # Giriş noktası — ardışık düzeni yönetir
│   ├── token.h             # Token türü tanımları
│   ├── lexer.h / lexer.c   # Sözcük çözümleyici — DURATION_LIT, WEIGHT_LIT, anahtar sözcükler
│   ├── parser.h / parser.c # Özyinelemeli iniş ayrıştırıcı — AST + hata mesajları
│   ├── ast.h / ast.c       # AST düğüm tanımları (etiketli birleşim) + yazdırıcı
│   ├── symbol_table.h / symbol_table.c  # Ortam zinciri — statik kapsam
│   ├── type_checker.h / type_checker.c  # İki geçişli statik tür denetleyicisi
│   └── interpreter.h / interpreter.c    # Ağaç yürüyüşlü yorumlayıcı
└── tests/
    ├── valid1.wl           # Üst vücut antrenmanı — if/else, repeat
    ├── valid2.wl           # Bacak günü — while döngüsü, fonksiyonlar
    ├── valid3.wl           # Tam vücut — bool fonksiyon, &&, !
    ├── error1.wl .. error5.wl       # Ayrıştırıcı hata durumları
    └── type_error1.wl .. type_error3.wl  # Tür / çalışma zamanı hataları
```

---

## Ardışık Düzen (Pipeline)

```
kaynak.wl
    │
    ▼
  Sözcük Çözümleyici   →  Token akışı
    │
    ▼
  Ayrıştırıcı          →  AST (satır numarasıyla)
    │
    ▼
  Tür Denetleyicisi    →  1. Geçiş: global isimleri kaydet
  (iki geçiş)          →  2. Geçiş: tüm deyim gövdelerini denetle
    │
    ▼
  Yorumlayıcı          →  stdout / stderr
```

---

## Hata Biçimi

Tüm hatalar şu biçimi kullanır:

```
[line N] <Hata türü>: <mesaj>
```

Örnekler:
```
[line 3] Parse error at '10': expected 'x' between sets and reps
[line 3] Type error: cannot initialise 'result' (type bool) with expression of type int
[line 2] Runtime error: division by zero
```

---

## Temel Tasarım Kararları

**Örtük tür dönüşümü yok.** Her operandın beklenen türde olması zorunludur. `int → float` genişlemesine bile izin verilmez. Bu karar, programlama bilgisi olmayan kullanıcılar için sessiz hata sınıfını tamamen ortadan kaldırır.

**`duration` ve `weight` yalnızca karşılaştırma operatörleri alır.** Bu türlerde aritmetik (`90s + 60s`) tür denetleyicisi tarafından engellenir. İki dinlenme süresini toplamak makul olabilir; iki ağırlığı çarpmak (kg²) ise boyutsal anlam taşımaz.

**Atama bir deyimdir, ifade değil.** `if (x = 5)` WorkoutLang'de ayrıştırma hatasıdır. `=` ve `==` sözdizimsel olarak ayrılmıştır; `=` yalnızca `var_decl` ve `assign_stmt` bağlamında geçerlidir.

**`repeat N weeks` iterasyon sayacı sunmaz.** Döngü gövdesi hangi haftada olduğunu bilemez. Bu kısıtlama, indeks aritmetiği hatalarını önler ve semantiği koçların döngüsel antrenman hakkındaki zihinsel modeliyle örtüştürür.

**Ortam zinciri ile statik kapsam.** Kapsam çerçeveleri bağlantılı listeden oluşur. `scope_lookup`, en içteki çerçeveden en dıştakine doğru yürür. Her `if`, `while` ve `repeat` gövdesi yeni bir `SCOPE_BLOCK` çerçevesi açar; çerçeve, blok çıkışında yok edilir (yığın dinamik yaşam süresi).

---

## Bilinen Sınırlamalar

- `Day.setCount` nokta notasyonu dil gramerinde tanımlanmıştır ancak henüz ayrıştırıcıya eklenmemiştir. Tür denetleyicisi ve yorumlayıcı bu yolu destekler; `parse_primary` içine ~10 satır eklenmesi yeterlidir.
- `return` deyimi olmadan biten bir `func`, sessizce `0` döndürür. Bunu tespit etmek, mevcut iki geçişli tür denetleyicisinin ötesinde kontrol akışı analizi gerektirir.
- `weight` türü yalnızca tamsayı literalleri kabul eder (`80kg`, `3.5kg` değil). Dahili olarak lb→kg dönüşümü için `double` olarak saklanır; çıktı her zaman kg cinsindendir.

---

## Akademik Bağlam

| | |
|---|---|
| Ders | CSE 341 — Programlama Dilleri Kavramları |
| Dönem | Bahar 2026 |
| Kaynak | Robert W. Sebesta, *Concepts of Programming Languages*, 12. baskı |
