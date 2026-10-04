# Kişi 1 kabul ve doğrulama kaydı

Tarih: 4 Ekim 2026. Kaynak gereksinimler: `SIMDIS_Dinamik_Risk_Bolgesi_Teknik_Raporu.pdf/.docx` ve `SIMDIS_Es_Zamanli_Gelistirme_Teknik_Raporu.pdf`. Belgeler gereksinim kaynağı olarak incelendi; üç kişilik dağılıma göre yalnızca kişi 1'in uygulaması ve ana entegrasyonu geliştirildi.

Aşağıdaki numaralar **Eş Zamanlı Geliştirme Teknik Raporu, bölüm 18** ile aynıdır.

| Senaryo | Beklenen | Kanıt / sonuç |
| --- | --- | --- |
| T-01 Risk kapalı | Tüm İHA'lar SAFE; rota değişmez | TestDemoIntegration + before-route.png: geçti |
| T-02 Risk açık ve uzakta | Rota değişmez | TestDemoIntegration: geçti |
| T-03 Yalnız İHA-2 kesişimi | Yalnız İHA-2 yeniden rotalanır | TestDemoIntegration + violation.png/after-route.png: geçti |
| T-04 Güvensiz aday | Validator false; rota uygulanmaz | İlk planlayıcı adayı 500 m payı ihlal ettiği için reddedildi; konum/revizyon değişmedi: geçti |
| T-05 Güvenli aday | setRoute ile uygulanır | Her iki mevcut güvenlik modülü; bağlantı dahil bütün rota; 600 uçuş örneği; after-route.png: geçti |
| T-06 Yarıçap değişimi | Kalan rota yeni yarıçapa göre değerlendirilir | Mevcut detour'u gerçekten kesen büyütme; tekrar planlama veya güvenlik beklemesi; küçültme: geçti |
| T-07 Risk kapatma | Yeni ihlal üretilmez | Aktif bölge kapatıldı; tüm durumlar SAFE; planlama sayısı artmadı: geçti |
| T-08 Diğer İHA'lar | İHA-1/3 başlangıç rotaları korunur | Atanmış rotalar ve revizyonlar değişmedi; üç İHA da hedefe vardı: geçti |

İlk teknik rapordaki T1-T7 de bu kontroller ve ek stabilite testiyle karşılanır: arka arkaya üç reset/tekrar; 600 karede tek rota uygulaması; blokta sabit deneme sayısı; VIOLATION bildirim aralığı ve ardından REROUTED; güvensiz başlangıçta anında güvenlik beklemesi.

Platform testinde ek kontroller: üç gerçek SIMDIS entity'si; ECEF DataStore kaydının public WGS84 state ile uyuşması; hız x zaman; waypoint tüketimi; konumu sıçratmadan setRoute; yanlış girdide atomik ret; zamanın geriye gitmesinin reddi; varışta hız sıfır; büyük/küçük zaman adımlarının aynı sonuç vermesi; hız değişiminde süreklilik.

**5 test grubunun tamamı başarılı.** Derleme Release/x64. Görsel test gerçek viewer, gerçek platformlar ve mevcut Cemre/Berra kaynakları ile çalıştırıldı. Test penceresi otomatik kapandı; normal demo bu şekilde kapanmaz.

Görsel incelemede Windows DPI nedeniyle sağ panelin dışarı taşması bulundu. Kişi 1 pencere adaptörü ImGui mantıksal pencere genişliğine göre sağa sabitlendi; arkadaşların UI kaynaklarına dokunulmadı.

Bu kayıt sabit demo ve test edilen senaryoların doğruluğunu gösterir; tüm coğrafi konumlar/tehdit geometrileri veya tam bitirme projesinin tamamlandığı iddiası değildir.

Klasör ayrımı doğrulaması (4 Ekim 2026): Proje `C:/Users/cagla/Desktop/Projeler/MERGEN` konumuna taşındı; SDK `C:/Users/cagla/Desktop/SimdisSDK` altında kaldı. Yeni konumda temiz Release derleme, 5/5 test ve gerçek SIMDIS/OpenGL görüntü testi başarılı. Eski derleme yedeği temizlendi. Kaynak dosyaları ve bütün Git referansları taşıma öncesi kayıtla karşılaştırıldı; korundukları doğrulandı. Test kayıtları projenin `logs/` klasöründedir.
