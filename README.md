# MERGEN - Kişi 1 demo teslimi

SIMDIS SDK 1.25 ile üç İHA'nın gerçek 3B uçuşu ve dinamik risk bölgesi demosu. Kişi 1'in platform ve ana uygulama kodu `feature/platform-simulation` dalında geliştirildi; tamamlanan ortak demo `main` dalında yayımlanır. `cemre` ve `berra` dalları korunur.

## Çalıştırma

`Start-Demo.cmd` dosyasına çift tıklayın. EXE'yi tek başına açmayın: başlatıcı gerekli DLL ve harita yollarını kendi sürecinde ayarlar.

1. Üç İHA'nın farklı hız ve irtifalarda uçtuğunu izleyin.
2. Sağ panelde **Activate Risk Zone** düğmesine başlangıçta basın. Varsayılan merkez yalnızca İHA-2'nin orijinal rotasını keser.
3. İHA-2 önce **VIOLATION**, ardından **REROUTED** olur. Yeşil çizgi doğrulanmış yeni rotadır; ince renkli çizgiler orijinal rotalardır. Kırmızı halka gerçek risk, sarı halka 500 metre güvenlik payıdır.
4. **Radius (m)** ile yarıçapı değiştirin; kalan rota yeniden değerlendirilir. Sol panelde **Risk zone center** merkez koordinatlarını düzenler.
5. **Reset scenario** başlangıç konumlarını, orijinal rotaları, hızları, geçmiş izleri, risk durumunu ve planlama sayaçlarını sıfırlar. Demo tekrar çalıştırılabilir.

Risk bölgesi mevcut konumun veya hedefin üstüne konursa ya da güvenli rota bulunamazsa **SAFETY HOLD** gösterilir ve simülasyon durur. Bölgeyi kapatmak veya uygun yere taşımak güvenlik beklemesini kaldırır. **Pause** kullanıcının ayrı duraklatma kontrolüdür. Normal uygulama siz kapatana kadar açık kalır.

Çevrimdışı SDK Hawaii haritası kullanılır; test senaryosu Kauai çevresindedir. Rapordaki örnek coğrafi koordinatlar yerine aynı üç-İHA geometrisi bu haritaya taşınmıştır. İnternet harita servisi gerekmez.

## Derleme ve test

Bu klasörde PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Test.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Run.ps1 -SmokeTest
```

Windows x64, Visual Studio Community 2026 C++ araçları ve CMake kullanılır. Proje `C:\Users\cagla\Desktop\Projeler\MERGEN` altında; ortak SDK projenin dışında `C:\Users\cagla\Desktop\SimdisSDK` altındadır. Bu SDK klasörünün `install`, `sdk` ve `data/SIMDIS_SDK-Data` alt klasörleri kullanılır. Yeni SIMDIS projeleri de aynı kurulumu kullanabilir.

`scripts/SdkPaths.ps1` tüm başlatıcılar için SDK yollarını tek yerden çözer. Varsayılan `$env:USERPROFILE\Desktop\SimdisSDK`; başka kurulum için Build/Run/Test betiklerine `-SdkHome 'D:\SDKs\SimdisSDK'` verin veya yalnızca mevcut terminalde `$env:SIMDIS_SDK_HOME` tanımlayın. `-SdkRoot`, `-SdkSource` ve `-DataRoot` ilgili betiklerde ayrı ayrı da belirtilebilir. CMake GUI için `SIMDIS_HOME` (ortak kurulum klasörü), gerekirse `SIMDIS_SDK_ROOT`, `SIMDIS_SDK_SRC_DIR`, `SIMDIS_THIRD_PARTY_DIR` ayarlanabilir. Kalıcı PATH veya sistem ayarı değişmez. Başka bilgisayarda bu SDK/bağımlılık/veri klasörleri de gerekir; yalnızca bu depoyu kopyalamak yeterli değildir.

Görsel test 220 kare sonunda otomatik kapanır, `artifacts/before-route.png`, `violation.png`, `after-route.png` dosyalarını üretir. Başarılı olması için yalnızca İHA-2'ye bir yeni rota uygulanmış olmalıdır. Normal başlatmada bu otomatik kapanış yoktur.

## Kişi 1'in sağladıkları

| Dosya | Sorumluluk |
| --- | --- |
| `Platform/PlatformManager.h/.cpp` | İHA oluşturma, state, kalan rota, sürekli hareket, uçuş ortasında rota atama, SIMDIS DataStore güncellemeleri, reset |
| `Platform/RouteGraphics.h/.cpp` | Orijinal ve değiştirilmiş rotaların gerçek SIMDIS çizimleri |
| `Platform/RiskGraphics.h/.cpp` | Kişi 2'nin RiskZoneData bilgisini 3B sahnede gösterme |
| `Platform/RouteApplication.h` | Mevcut konumdan aday rotaya bağlantı dahil doğrulama; başarısızsa hiçbir rota değişmez |
| `Integration/DemoScenario.h/.cpp` | Üç farklı İHA'nın başlangıç konumları, hızları ve waypoint rotaları |
| `Integration/DemoController.h/.cpp` | Kişi 2 ve kişi 3'ün mevcut API'lerini ana döngüye bağlama; sınırlı planlama tekrarları; güvenlik beklemesi |
| `main.cpp`, `CMakeLists.txt`, `scripts/` | Viewer, paneller, SDK uyumluluğu, derleme/çalıştırma/test başlatıcıları |
| `Tests/TestPlatformManager.cpp`, `Tests/TestDemoIntegration.cpp` | Gerçek DataStore ve modüller arası davranış doğrulaması |

Ortak `CommonTypes.h` alan/adları değiştirilmedi. `PlatformManager` dışına SIMDIS nesneleri gönderilmez; public state/route API'si standart C++ veri tipleri kullanır.

```cpp
int createPlatform(const std::string& name, const Waypoint& start);
AircraftState getState(int platformId) const;
std::vector<Waypoint> getRoute(int platformId) const;
void setRoute(int platformId, const std::vector<Waypoint>& route);
void update(double simulationTime);
```

Birimler: WGS84 derece, metre (elipsoid irtifası), saniye, m/s; heading kuzeyden saat yönünde derece. Zaman geriye gidemez; `reset()` hariç monoton artar. `getRoute()` **güncel konum + ziyaret edilmemiş waypoint'ler** döndürür; varışta tek nokta kalır. `setRoute()` konumu sıçratmaz ve tüm girdiyi değiştirmeden önce yapısal olarak doğrular. Risk güvenliğini tek başına garanti etmez; ana uygulama önce `applyValidatedRoute` kullanır. Yerel düzlem yaklaşımı ve en fazla 100 km rota bacağı demo içindir; dünya ölçeğinde uçuş modeli değildir.

## Arkadaşların kodları ve uyumluluk

Kaynak: https://github.com/berramiynat-debug/MERGEN

- **Cemre / kişi 2**: `a57ffa2`, `Risk/RiskZone.*`, `Risk/ViolationDetector.*`, `Tests/TestRiskZone.cpp` aynen yerel kişi 1 dalına alındı.
- **Berra / kişi 3**: `4594c3f`, `Navigation/RoutePlanner.*`, `Navigation/RouteValidator.*`, `UI/ControlPanel.*` mevcut kaynak olarak korunuyor.
- SIMDIS 1.25 ImGui tabanı `draw()` beklerken Berra'nın paneli `drawContents_()` kullanıyor. CMake, yalnızca `build/Person3UI` altında uyumlu bir derleme kopyası üretir; kaynak UI dosyalarında değişiklik yoktur. Pencere çerçevesi/ölçeklendirme kişi 1'in `main.cpp` adaptöründedir.
- Berra'nın ilk detour adayı 500 metre pay ile doğrulamadan geçmeyebiliyor. Algoritması değiştirilmedi. Ana uygulama aynı planlayıcıyı 500, 1000, 2000, 4000 ve 8000 metre aday paylarıyla en çok beş kez çağırır. Kabul için **hem Cemre'nin detector'ü genişletilmiş bölgede SAFE hem Berra'nın validator'ü gerçek bölge + 500 metre payda güvenli** olmalıdır. Hiçbiri uygun değilse uçuş durur.
- Aynı bölge ve rota için her karede tekrar planlama yapılmaz; değişen bölge veya harici rota ataması tekrar değerlendirilir. Risk tespiti yalnızca kalan rota üzerinde çalışır.

## Doğrulama sonucu

4 Ekim 2026: Release/x64 derleme başarılı; **5/5 CTest grubu geçti** (`TestRoutePlanner`, `TestRouteValidator`, `TestRiskZone`, `TestPlatformManager`, `TestDemoIntegration`). Assert kullanan mevcut testler Release modunda da etkin. Gerçek OpenGL/SIMDIS görsel test başarılı; üç görüntü açılarak incelendi. Kabul senaryosu ayrıntıları `KABUL_TESTLERI.md` dosyasındadır.

Yerel kayıtlar `logs/` klasöründe: `MERGEN-build.log`, `MERGEN-tests.log`, `MERGEN-render.log`, `MERGEN-source-check.log`. Son dosya, Windows CRLF/LF satır sonları normalleştirildikten sonra arkadaşların 12 kaynak dosyasının Git commit içerikleriyle aynı olduğunu SHA256 ile kaydeder. İlk başarısız yarıçap testindeki varsayım düzeltilip, yeni yarıçapın mevcut rotayı gerçekten kestiği ayrıca doğrulandı; son test kaydı başarılı çalışmayı içerir.

## Kapsam sınırı

Bu teslim raporlardaki demo ve kişi 1 kapsamıdır. Risk bölgesi irtifadan bağımsız yatay bir dairedir; üç boyutlu tehdit hacmi veya aerodinamik uçuş modeli uygulanmadı. Sabit demo dışında her geometride güvenli detour üretme garantisi yoktur; reddetme ve güvenlik beklemesi uygulanır. Bölgenin aniden İHA'nın üzerine taşınması geçmiş ihlali geri alamaz.

LLM/LoRA, ns-3, Zero Trust, çoklu İHA çarpışma çözümü ve görev kurtarma tam bitirme projesinin sonraki aşamasıdır; bu demo kapsamında kurulması/geliştirilmesi gerekmez. Depo kaynak kodu ve başlatıcıları içerir; SDK kurulumu, haritalar ve derleme çıktıları yerel ortak SDK klasöründe tutulur.


## Klasör ayrımı (4 Ekim 2026)

Proje, Git geçmişi ve kişi 1'in çalışma dosyalarıyla `Masaüstü/Projeler/MERGEN` konumuna ayrıldı. SDK kurulumu ve kendi örnek başlatıcıları `Masaüstü/SimdisSDK` altında kaldı. Projenin `build` klasörü yeni konum için yeniden oluşturuldu; kalıcı sistem ayarı değişmedi. SDK örneklerini bağımsız açmak için `SimdisSDK/Start-Simdis.ps1` kullanılmaya devam edilir. `cemre` ve `berra` dal/commit'lerinde değişiklik yapılmadı.
