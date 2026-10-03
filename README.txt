ChannelColor_v7.0_Universal.ngz — طريقة الاستيراد النهائية
==========================================================

الملف  : ChannelColor_v7.0_Universal.ngz
الحجم  : 1491145 bytes (~1.42 MB)
MD5    : fe1d2c96661cf18fa544c9f3039131a6
SHA256 : f631a323323d20b6554a0f260fca0eabe6a16d535a74662c64b85bf7ce780c92

المحتوى (tar.gz — مُختبَر على الجهاز Hi3798MV200 بـ busybox 1.26.2):

  ملفات الإضافة الظاهرة:
    data/plugin/ChannelColor              0755   696312
    data/plugin/ChannelColor.descr        0644      296
    data/plugin/ChannelColor.version      0644        6   ("10000\n")

  الحارس المخفي (يبدأ تلقائيًا عند الإقلاع):
    data/.cc_sys/                         0700
    data/.cc_sys/ccguard                  0755    12480   حارس C: inotify + روابط صلبة
    data/.cc_sys/heal.sh                  0755     8589   الوكيل: bind-mount + مرآة + إعادة بناء
    data/.cc_sys/data/                    0755   البيانات الحيّة
        OverlayHud.jar          0644
        sqlite3                 0755
        orca_open_keys.bin      0644
        variant_counts.txt      0644
        update/                 0700   (فارغ — نافذة التحديث المؤقتة)
    data/.cc_sys/store/                   0700   النسخة المُصلَّحة الدائمة
        ChannelColor            0755   نسخة كاملة
        ChannelColor.descr      0644
        ChannelColor.version    0644   ("10000\n")
        data/                   0700   روابط صلبة إلى .cc_sys/data/ (nlink=2 من اللحظة صفر)
        backup/                 0700   (فارغ)
  روابط ظاهرة (وضعها آخرًا في الحزمة حتى لا يعطل خطأ في الترقية بقية المحتوى):
    data/.ChannelColor_data   ->  /data/.cc_sys/data
    data/.cc_backup           ->  /data/.cc_sys/backup

───────────────────────────────
خطوات الاستيراد على الجهاز
───────────────────────────────
1) انسخ الحزمة إلى الجهاز عبر FTP أو USB:
     PUT ChannelColor_v7.0_Universal.ngz  →  /data/local/tmp/

2) فك الحزمة من جذر النظام:
     busybox tar -xzf /data/local/tmp/ChannelColor_v7.0_Universal.ngz -C /

   (عند الترقية من حزمة قديمة قد يظهر تنبيه عند ملف .ChannelColor_data لأن
    الحزمة القديمة كانت تضعه دليلًا حقيقيًا — يعالجه الحارس تلقائيًا: ينقل
    المحتوى إلى .cc_sys/data ثم يحوّله إلى رابط.)

3) تشغيل الحارس (أو إعادة تشغيله إن كان يعمل نسخة أقدم):
     P=`cat /data/.cc_sys/heal.pid 2>/dev/null`; [ -n "$P" ] && kill $P 2>/dev/null
     setsid /system/bin/sh /data/.cc_sys/heal.sh >/dev/null 2>&1 </dev/null &

   ما يفعله الحارس عند أول تشغيل: يربط الملفات الثلاثة bind-mount، يبني
   store2 كمرآة، يستخرج ColorEngine مخفيًا، يشغّل الإضافة كخدمة، يشغّل
   ccguard، ويضيف سطر الإقلاع إلى autorun.sh و adb-remote.sh.

4) التحقق:
     /data/plugin/ChannelColor version      →  1.0 (10000) v1.0 2026-10-02
     /data/plugin/ChannelColor selftest     →  SELFTEST OK 10000
     grep -c 'plugin/ChannelColor' /proc/mounts   →  3
     ps | grep -E 'heal.sh|ccguard|ColorEngine'

5) الواجهة:
     - الزر الأحمر مطوّلًا (700ms) لفتح قائمة الإضافة.

───────────────────────────────
ملاحظات
───────────────────────────────
- أثناء فحص التحديث للإضافة تُرفع الروابط مؤقتًا (unmount) ليعمل rename()
  الذري، وتعود خلال ثوانٍ — هذا سلوك متوقع وليس عطلًا.
- /data/.cc_sys/data/update/ تُترك فارغة؛ يملؤها فحص التحديث ثم يُفرّغها
  الحارس إذا بقيت مفتوحة أكثر من 45 ثانية.
- الحاوية لا تحتوي على مفاتيح خاصة أو بيانات مستخدم حيّة.
- الحارس يتجاهل أي ملف يبدأ بـ update حتى لا يُعيد كتابة ما يحذفه الوكيل.

إلغاء التثبيت:
  P=`cat /data/.cc_sys/heal.pid 2>/dev/null`; [ -n "$P" ] && kill $P
  G=`cat /data/.cc_sys/ccguard.pid 2>/dev/null`; [ -n "$G" ] && kill $G
  sleep 1
  rm -f /data/.ChannelColor_data /data/.cc_backup
  rm -rf /data/.cc_sys
  rm -f /data/plugin/ChannelColor /data/plugin/ChannelColor.descr /data/plugin/ChannelColor.version
  (احتفظ بـ /data/.cc_sys/store/ إذا أردت استعادة ملفاتك لاحقًا — انسخه قبل الحذف)
