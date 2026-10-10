package lab.scoreboard;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.*;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.*;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.*;
import java.security.cert.X509Certificate;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import javax.net.ssl.*;

public class ScoreBoardHud {
    private static final String TAG = "ScoreBoardHud";
    private static WindowManager mWindowManager;
    private static WindowManager.LayoutParams mParams;
    private static TickerView mTickerView;
    private static boolean mIsVisible = true;
    private static Handler mHandler;

    public static class ScoreConfig {
        public boolean enabled = true;
        public boolean showUcl = true;
        public boolean showEpl = true;
        public boolean showLaliga = true;
        public boolean showSeriea = true;
        public boolean showArab = true;
        public boolean liveOnly = false;
        public boolean showDetails = true;

        public static ScoreConfig load() {
            ScoreConfig cfg = new ScoreConfig();
            File f1 = new File("/data/plugin/ColorPro_data/scoreboard_cfg.json");
            File f2 = new File("/data/plugin/scoreboard_cfg.json");
            File f = f1.exists() ? f1 : (f2.exists() ? f2 : null);
            if (f != null) {
                try {
                    BufferedReader br = new BufferedReader(new FileReader(f));
                    StringBuilder sb = new StringBuilder();
                    String line;
                    while ((line = br.readLine()) != null) sb.append(line);
                    br.close();
                    JSONObject json = new JSONObject(sb.toString());
                    cfg.enabled = json.optBoolean("enabled", true);
                    cfg.showUcl = json.optBoolean("ucl", true);
                    cfg.showEpl = json.optBoolean("epl", true);
                    cfg.showLaliga = json.optBoolean("laliga", true);
                    cfg.showSeriea = json.optBoolean("seriea", true);
                    cfg.showArab = json.optBoolean("arab", true);
                    cfg.liveOnly = json.optBoolean("live_only", false);
                    cfg.showDetails = json.optBoolean("show_details", true);
                } catch (Exception e) {
                    log("Config load error: " + e.getMessage());
                }
            }
            return cfg;
        }
    }

    private static ScoreConfig mConfig = new ScoreConfig();

    public static class MatchItem {
        public String leagueKey;
        public String competition;
        public String team1;
        public int score1;
        public int score2;
        public String team2;
        public String statusText;
        public boolean isLive;
        public String scorers;
        public String yellowCards;
        public String redCards;
        public String logo1Url;
        public String logo2Url;
        public Bitmap logo1;
        public Bitmap logo2;

        public MatchItem(String leagueKey, String comp, String t1, int s1, int s2, String t2,
                         String statusText, boolean live, String scorers, String yellowCards, String redCards,
                         String logo1Url, String logo2Url) {
            this.leagueKey = leagueKey;
            this.competition = comp;
            this.team1 = t1;
            this.score1 = s1;
            this.score2 = s2;
            this.team2 = t2;
            this.statusText = statusText;
            this.isLive = live;
            this.scorers = scorers;
            this.yellowCards = yellowCards;
            this.redCards = redCards;
            this.logo1Url = logo1Url;
            this.logo2Url = logo2Url;
        }
    }

    private static final List<MatchItem> mAllMatches = new ArrayList<MatchItem>();
    private static final List<MatchItem> mDisplayMatches = Collections.synchronizedList(new ArrayList<MatchItem>());

    /* ---- Logo Cache and Downloader ---- */
    private static final Map<String, Bitmap> sLogoCache = new ConcurrentHashMap<String, Bitmap>();
    private static final File LOGO_DIR = new File("/data/plugin/ColorPro_data/logos");

    public static Bitmap getLogo(String url) {
        if (url == null || url.trim().isEmpty()) return null;
        if (sLogoCache.containsKey(url)) return sLogoCache.get(url);

        try {
            if (!LOGO_DIR.exists()) LOGO_DIR.mkdirs();
            String safeName = url.replaceAll("[^a-zA-Z0-9_\\-\\.]", "_");
            if (safeName.length() > 64) safeName = Math.abs(url.hashCode()) + ".png";
            File localFile = new File(LOGO_DIR, safeName);
            if (localFile.exists() && localFile.length() > 100) {
                Bitmap bmp = BitmapFactory.decodeFile(localFile.getAbsolutePath());
                if (bmp != null) {
                    Bitmap scaled = Bitmap.createScaledBitmap(bmp, 28, 28, true);
                    sLogoCache.put(url, scaled);
                    return scaled;
                }
            }
        } catch (Throwable ignored) {}
        return null;
    }

    private static void downloadOneLogo(String url) {
        if (url == null || url.trim().isEmpty()) return;
        try {
            if (!LOGO_DIR.exists()) LOGO_DIR.mkdirs();
            String safeName = url.replaceAll("[^a-zA-Z0-9_\\-\\.]", "_");
            if (safeName.length() > 64) safeName = Math.abs(url.hashCode()) + ".png";
            File localFile = new File(LOGO_DIR, safeName);
            if (localFile.exists() && localFile.length() > 100) {
                if (!sLogoCache.containsKey(url)) {
                    Bitmap bmp = BitmapFactory.decodeFile(localFile.getAbsolutePath());
                    if (bmp != null) {
                        Bitmap scaled = Bitmap.createScaledBitmap(bmp, 28, 28, true);
                        sLogoCache.put(url, scaled);
                    }
                }
                return;
            }

            HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
            conn.setConnectTimeout(4000);
            conn.setReadTimeout(5000);
            conn.setRequestProperty("User-Agent", "Mozilla/5.0");
            conn.connect();
            if (conn.getResponseCode() == 200) {
                InputStream in = conn.getInputStream();
                File tmpFile = new File(LOGO_DIR, safeName + ".tmp");
                FileOutputStream fos = new FileOutputStream(tmpFile);
                byte[] buf = new byte[4096];
                int r;
                while ((r = in.read(buf)) != -1) fos.write(buf, 0, r);
                fos.close();
                in.close();
                tmpFile.renameTo(localFile);

                Bitmap bmp = BitmapFactory.decodeFile(localFile.getAbsolutePath());
                if (bmp != null) {
                    Bitmap scaled = Bitmap.createScaledBitmap(bmp, 28, 28, true);
                    sLogoCache.put(url, scaled);
                }
            }
            conn.disconnect();
        } catch (Throwable ignored) {}
    }

    public static class ScrollTask implements Runnable {
        private final TickerView mView;
        public ScrollTask(TickerView view) {
            this.mView = view;
        }
        @Override
        public void run() {
            if (mView.mRunning && mIsVisible) {
                mView.mScrollX += mView.mSpeed;
                if (mView.mPeriod > 0 && mView.mScrollX >= mView.mPeriod) {
                    mView.mScrollX %= mView.mPeriod;
                }
                mView.invalidate();
                mView.postDelayed(this, 16);
            }
        }
    }

    public static class TickerView extends View {
        public final Paint mPaintText = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintScore = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintScoreBg = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintScoreBorder = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintBadge = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintBadgeBg = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintBadgeBorder = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintStatus = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintStatusBg = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintScorers = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintYellowCard = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintRedCard = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintCardBorder = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintYellowText = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintRedText = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintGoalBadge = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintGoalText = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintLine = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintStripe = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintGrid = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintBg = new Paint();
        public final Paint mPaintGlow = new Paint(Paint.ANTI_ALIAS_FLAG);
        public final Paint mPaintCore = new Paint(Paint.ANTI_ALIAS_FLAG);

        public float mScrollX = 0f;
        public float mSpeed = 2.0f;
        public boolean mRunning = true;
        public float mPeriod = 0f;
        public boolean mNeedMeasure = true;
        public final ScrollTask mScrollRunnable;

        public TickerView(Context ctx) {
            super(ctx);
            mScrollRunnable = new ScrollTask(this);

            // 1. Teams text (White Bold)
            mPaintText.setColor(Color.WHITE);
            mPaintText.setTextSize(21f);
            mPaintText.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

            // 2. Score text (Vivid Yellow)
            mPaintScore.setColor(Color.parseColor("#FFFF00"));
            mPaintScore.setTextSize(22f);
            mPaintScore.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

            mPaintScoreBg.setColor(Color.parseColor("#33FFD700"));
            mPaintScoreBg.setStyle(Paint.Style.FILL);

            mPaintScoreBorder.setColor(Color.parseColor("#80FFD700"));
            mPaintScoreBorder.setStyle(Paint.Style.STROKE);
            mPaintScoreBorder.setStrokeWidth(1.2f);

            // 3. League Badge (Cyan Neon)
            mPaintBadge.setColor(Color.parseColor("#00E5FF"));
            mPaintBadge.setTextSize(16f);
            mPaintBadge.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

            mPaintBadgeBg.setColor(Color.parseColor("#2600E5FF"));
            mPaintBadgeBg.setStyle(Paint.Style.FILL);

            mPaintBadgeBorder.setColor(Color.parseColor("#8000E5FF"));
            mPaintBadgeBorder.setStyle(Paint.Style.STROKE);
            mPaintBadgeBorder.setStrokeWidth(1.2f);

            // 4. Status / Live Minute
            mPaintStatus.setColor(Color.parseColor("#00E676")); // Neon Green
            mPaintStatus.setTextSize(16f);
            mPaintStatus.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

            mPaintStatusBg.setColor(Color.parseColor("#2600E676"));
            mPaintStatusBg.setStyle(Paint.Style.FILL);

            // 5. Goal badge & Scorers text
            mPaintGoalBadge.setColor(Color.parseColor("#33FFD700"));
            mPaintGoalBadge.setStyle(Paint.Style.FILL);

            mPaintGoalText.setColor(Color.parseColor("#FFD700"));
            mPaintGoalText.setTextSize(14f);
            mPaintGoalText.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

            mPaintScorers.setColor(Color.parseColor("#FFE082"));
            mPaintScorers.setTextSize(16f);
            mPaintScorers.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.NORMAL));

            // 6. Vector Cards (Yellow & Red)
            mPaintYellowCard.setColor(Color.parseColor("#FFEB3B"));
            mPaintYellowCard.setStyle(Paint.Style.FILL);

            mPaintRedCard.setColor(Color.parseColor("#FF1744"));
            mPaintRedCard.setStyle(Paint.Style.FILL);

            mPaintCardBorder.setColor(Color.parseColor("#55000000"));
            mPaintCardBorder.setStyle(Paint.Style.STROKE);
            mPaintCardBorder.setStrokeWidth(1.0f);

            mPaintYellowText.setColor(Color.parseColor("#FFF59D"));
            mPaintYellowText.setTextSize(16f);
            mPaintYellowText.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.NORMAL));

            mPaintRedText.setColor(Color.parseColor("#FF5252"));
            mPaintRedText.setTextSize(16f);
            mPaintRedText.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

            // 7. Futuristic Top & Bottom Cyber borders
            mPaintLine.setColor(Color.parseColor("#00E5FF"));
            mPaintLine.setStrokeWidth(2.5f);

            mPaintStripe.setColor(Color.parseColor("#6600E5FF"));
            mPaintStripe.setStrokeWidth(1.8f);

            mPaintGrid.setColor(Color.parseColor("#2000E5FF"));
            mPaintGrid.setStyle(Paint.Style.FILL);

            mPaintGlow.setStyle(Paint.Style.FILL);
            mPaintCore.setColor(Color.WHITE);
            mPaintCore.setStyle(Paint.Style.FILL);

            File fontFile = new File("/data/plugin/hispf.ttf");
            if (fontFile.exists()) {
                try {
                    Typeface tf = Typeface.createFromFile(fontFile);
                    mPaintText.setTypeface(tf);
                    mPaintScorers.setTypeface(tf);
                    mPaintYellowText.setTypeface(tf);
                    mPaintRedText.setTypeface(tf);
                } catch (Exception ignored) {}
            }
        }

        public void setSpeed(float speed) {
            this.mSpeed = speed;
        }

        public void start() {
            mRunning = true;
            post(mScrollRunnable);
        }

        public void stop() {
            mRunning = false;
            removeCallbacks(mScrollRunnable);
        }

        private void drawGlowDot(Canvas c, float cx, float cy, int color) {
            float r = 7f;
            RadialGradient g = new RadialGradient(cx, cy, r,
                    new int[] { color, 0x6600E5FF, 0x00000000 },
                    new float[] { 0f, 0.4f, 1f },
                    Shader.TileMode.CLAMP);
            mPaintGlow.setShader(g);
            c.drawCircle(cx, cy, r, mPaintGlow);
            mPaintGlow.setShader(null);
            c.drawCircle(cx, cy, 2f, mPaintCore);
        }

        private float measureTotalWidth() {
            float w = 0;
            synchronized (mDisplayMatches) {
                for (int i = 0; i < mDisplayMatches.size(); i++) {
                    MatchItem m = mDisplayMatches.get(i);
                    String badge = "[" + m.competition + "]";
                    w += mPaintBadge.measureText(badge) + 14 + 18;

                    String sText = m.isLive ? ("\u25cf " + m.statusText) : m.statusText;
                    w += mPaintStatus.measureText(sText) + 12 + 16;

                    // Logo 1 + Team 1
                    Bitmap l1 = (m.logo1 != null) ? m.logo1 : getLogo(m.logo1Url);
                    if (l1 != null) w += 28 + 8;
                    w += mPaintText.measureText(m.team1) + 12;

                    // Score
                    String scText = " " + m.score2 + " - " + m.score1 + " ";
                    w += mPaintScore.measureText(scText) + 12 + 12;

                    // Team 2 + Logo 2
                    w += mPaintText.measureText(m.team2) + 8;
                    Bitmap l2 = (m.logo2 != null) ? m.logo2 : getLogo(m.logo2Url);
                    if (l2 != null) w += 28 + 16;
                    else w += 8;

                    if (mConfig.showDetails && m.scorers != null && m.scorers.length() > 0) {
                        w += 44 + 8 + mPaintScorers.measureText(m.scorers) + 16;
                    }
                    if (mConfig.showDetails && m.yellowCards != null && m.yellowCards.length() > 0) {
                        w += 10 + 5 + mPaintYellowText.measureText(m.yellowCards) + 16;
                    }
                    if (mConfig.showDetails && m.redCards != null && m.redCards.length() > 0) {
                        w += 10 + 5 + mPaintRedText.measureText(m.redCards) + 16;
                    }
                    w += mPaintBadge.measureText("\u2756") + 28;
                }
            }
            return (w > 0) ? w : 1920f;
        }

        private void drawMatchTrain(Canvas canvas, float headX, int width, int height, float baseline) {
            if (headX < 0) return;
            float curX = headX;
            float logoY = (height - 28) / 2f;

            synchronized (mDisplayMatches) {
                for (int i = 0; i < mDisplayMatches.size(); i++) {
                    if (curX < -200) break;
                    MatchItem m = mDisplayMatches.get(i);

                    // 1. League Badge
                    String badge = "[" + m.competition + "]";
                    float bTextW = mPaintBadge.measureText(badge);
                    float bBoxW = bTextW + 14;
                    float bLeft = curX - bBoxW;
                    if (curX >= 0 && bLeft <= width) {
                        RectF r = new RectF(bLeft, 11, curX, height - 11);
                        canvas.drawRoundRect(r, 6, 6, mPaintBadgeBg);
                        canvas.drawRoundRect(r, 6, 6, mPaintBadgeBorder);
                        canvas.drawText(badge, bLeft + 7, baseline - 2, mPaintBadge);
                    }
                    curX = bLeft - 18;

                    // 2. Status / Minute
                    String sText = m.isLive ? ("\u25cf " + m.statusText) : m.statusText;
                    int sCol = m.statusText.contains("HT") ? Color.parseColor("#FFD700") :
                            (m.isLive ? Color.parseColor("#00E676") :
                            ((m.statusText.contains("FT") || m.statusText.contains("انتهت") || m.statusText.contains("نهاية")) ? Color.parseColor("#80D8FF") : Color.parseColor("#B0BEC5")));
                    mPaintStatus.setColor(sCol);
                    float sTextW = mPaintStatus.measureText(sText);
                    float sBoxW = sTextW + 12;
                    float sLeft = curX - sBoxW;
                    if (curX >= 0 && sLeft <= width) {
                        RectF r = new RectF(sLeft, 12, curX, height - 12);
                        mPaintStatusBg.setColor(Color.argb(40, Color.red(sCol), Color.green(sCol), Color.blue(sCol)));
                        canvas.drawRoundRect(r, 5, 5, mPaintStatusBg);
                        canvas.drawText(sText, sLeft + 6, baseline - 2, mPaintStatus);
                    }
                    curX = sLeft - 16;

                    // 3. Team 1 Logo & Name
                    Bitmap logo1 = (m.logo1 != null) ? m.logo1 : getLogo(m.logo1Url);
                    if (logo1 != null) {
                        float l1Left = curX - 28;
                        if (curX >= 0 && l1Left <= width) {
                            canvas.drawBitmap(logo1, null, new RectF(l1Left, logoY, curX, logoY + 28), null);
                        }
                        curX = l1Left - 8;
                    }

                    float t1W = mPaintText.measureText(m.team1);
                    float t1Left = curX - t1W;
                    if (curX >= 0 && t1Left <= width) {
                        canvas.drawText(m.team1, t1Left, baseline, mPaintText);
                    }
                    curX = t1Left - 12;

                    // 4. Score
                    String scText = " " + m.score2 + " - " + m.score1 + " ";
                    float scTextW = mPaintScore.measureText(scText);
                    float scBoxW = scTextW + 12;
                    float scLeft = curX - scBoxW;
                    if (curX >= 0 && scLeft <= width) {
                        RectF r = new RectF(scLeft, 10, curX, height - 10);
                        canvas.drawRoundRect(r, 6, 6, mPaintScoreBg);
                        canvas.drawRoundRect(r, 6, 6, mPaintScoreBorder);
                        canvas.drawText(scText, scLeft + 6, baseline, mPaintScore);
                    }
                    curX = scLeft - 12;

                    // 5. Team 2 Name & Logo
                    float t2W = mPaintText.measureText(m.team2);
                    float t2Left = curX - t2W;
                    if (curX >= 0 && t2Left <= width) {
                        canvas.drawText(m.team2, t2Left, baseline, mPaintText);
                    }
                    curX = t2Left - 8;

                    Bitmap logo2 = (m.logo2 != null) ? m.logo2 : getLogo(m.logo2Url);
                    if (logo2 != null) {
                        float l2Left = curX - 28;
                        if (curX >= 0 && l2Left <= width) {
                            canvas.drawBitmap(logo2, null, new RectF(l2Left, logoY, curX, logoY + 28), null);
                        }
                        curX = l2Left - 16;
                    } else {
                        curX = curX - 8;
                    }

                    // 6. Scorers
                    if (mConfig.showDetails && m.scorers != null && m.scorers.length() > 0) {
                        float scW = mPaintScorers.measureText(m.scorers);
                        float totalScW = 44 + 8 + scW;
                        float scL = curX - totalScW;
                        if (curX >= 0 && scL <= width) {
                            RectF goalBadge = new RectF(curX - 44, 14, curX, height - 14);
                            canvas.drawRoundRect(goalBadge, 5, 5, mPaintGoalBadge);
                            canvas.drawText("\u26bd", curX - 36, baseline - 2, mPaintGoalText);
                            canvas.drawText(m.scorers, scL, baseline - 1, mPaintScorers);
                        }
                        curX = scL - 16;
                    }

                    // 7. Yellow cards
                    if (mConfig.showDetails && m.yellowCards != null && m.yellowCards.length() > 0) {
                        float yW = mPaintYellowText.measureText(m.yellowCards);
                        float totalYW = 10 + 5 + yW;
                        float yL = curX - totalYW;
                        if (curX >= 0 && yL <= width) {
                            RectF yCard = new RectF(curX - 10, baseline - 15, curX, baseline + 1);
                            canvas.drawRoundRect(yCard, 2, 2, mPaintYellowCard);
                            canvas.drawRoundRect(yCard, 2, 2, mPaintCardBorder);
                            canvas.drawText(m.yellowCards, yL, baseline - 1, mPaintYellowText);
                        }
                        curX = yL - 16;
                    }

                    // 8. Red cards
                    if (mConfig.showDetails && m.redCards != null && m.redCards.length() > 0) {
                        float rW = mPaintRedText.measureText(m.redCards);
                        float totalRW = 10 + 5 + rW;
                        float rL = curX - totalRW;
                        if (curX >= 0 && rL <= width) {
                            RectF rCard = new RectF(curX - 10, baseline - 15, curX, baseline + 1);
                            canvas.drawRoundRect(rCard, 2, 2, mPaintRedCard);
                            canvas.drawRoundRect(rCard, 2, 2, mPaintCardBorder);
                            canvas.drawText(m.redCards, rL, baseline - 1, mPaintRedText);
                        }
                        curX = rL - 16;
                    }

                    // 9. Separator
                    float sepW = mPaintBadge.measureText("\u2756");
                    float sepL = curX - sepW;
                    if (curX >= 0 && sepL <= width) {
                        canvas.drawText("\u2756", sepL, baseline, mPaintBadge);
                    }
                    curX = sepL - 28;
                }
            }
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);

            int width = getWidth();
            int height = getHeight();
            if (width <= 0 || height <= 0) return;

            // Deep luxury gradient background
            mPaintBg.setShader(new LinearGradient(0, 0, 0, height,
                    new int[]{0xEE050B14, 0xFA091422, 0xFF050B14},
                    new float[]{0f, 0.5f, 1f}, Shader.TileMode.CLAMP));
            canvas.drawRect(0, 0, width, height, mPaintBg);

            // Subtle cyber tech grid
            for (float gy = 10; gy < height - 6; gy += 14) {
                canvas.drawRect(0, gy, width, gy + 1, mPaintGrid);
            }

            // Top neon cyan edge line with corner accents
            canvas.drawLine(0, 0, width, 0, mPaintLine);
            canvas.drawLine(0, 3, width, 3, mPaintStripe);

            // Left & Right Cyber Tech Caps
            canvas.drawRect(0, 0, 60, height, mPaintGrid);
            canvas.drawRect(width - 60, 0, width, height, mPaintGrid);

            // Animated pulsing corner glow orbs
            float pulsePhase = (System.currentTimeMillis() % 2000) / 2000f;
            int orbAlpha = (int)(180 + 75 * Math.sin(pulsePhase * Math.PI * 2));
            int cyanOrb = Color.argb(orbAlpha, 0, 229, 255);
            drawGlowDot(canvas, 16, 12, cyanOrb);
            drawGlowDot(canvas, width - 16, 12, cyanOrb);
            drawGlowDot(canvas, 16, height - 12, cyanOrb);
            drawGlowDot(canvas, width - 16, height - 12, cyanOrb);

            // Bottom subtle glowing accent line
            canvas.drawLine(0, height - 3f, width, height - 3f, mPaintLine);
            canvas.drawLine(0, height - 1f, width, height - 1f, mPaintStripe);

            if (mDisplayMatches.isEmpty()) {
                mPaintText.setColor(Color.parseColor("#90CAF9"));
                String waitMsg = "\u26bd \u062c\u0627\u0631\u064a \u062a\u062d\u062f\u064a\u062b \u0646\u062a\u0627\u0626\u064c \u0627\u0644\u0645\u0628\u0627\u0631\u064a\u0627\u062a \u0627\u0644\u0645\u0628\u0627\u0634\u0631\u0629 \u0645\u0639 \u0627\u0644\u0634\u0639\u0627\u0631\u0627\u062a...";
                float tw = mPaintText.measureText(waitMsg);
                canvas.drawText(waitMsg, (width - tw) / 2f, height * 0.65f, mPaintText);
                return;
            }

            float baseline = height * 0.64f;
            float totalW = measureTotalWidth();
            mPeriod = totalW + 200f;

            float offset = mScrollX % mPeriod;
            float curHead = offset;
            while (curHead - totalW < width) {
                if (curHead >= 0) {
                    drawMatchTrain(canvas, curHead, width, height, baseline);
                }
                curHead += mPeriod;
            }
        }
    }

    public static void show() {
        if (!mIsVisible && mTickerView != null) {
            mTickerView.setVisibility(View.VISIBLE);
            mTickerView.start();
            mIsVisible = true;
            log("ScoreBoardHud: shown");
        }
    }

    public static void hide() {
        if (mIsVisible && mTickerView != null) {
            mTickerView.stop();
            mTickerView.setVisibility(View.GONE);
            mIsVisible = false;
            log("ScoreBoardHud: hidden");
        }
    }

    public static void toggle() {
        if (mIsVisible) hide();
        else show();
    }

    public static void filterDisplayMatches() {
        List<MatchItem> filtered = new ArrayList<MatchItem>();
        synchronized (mAllMatches) {
            for (MatchItem m : mAllMatches) {
                if (mConfig.liveOnly && !m.isLive) continue;
                filtered.add(m);
            }
        }
        synchronized (mDisplayMatches) {
            mDisplayMatches.clear();
            mDisplayMatches.addAll(filtered);
        }
        if (mTickerView != null) {
            mTickerView.mNeedMeasure = true;
            mTickerView.postInvalidate();
        }
    }

    public static void reloadConfig() {
        mConfig = ScoreConfig.load();
        log("ScoreBoardHud: Reloaded config (enabled=" + mConfig.enabled + ", liveOnly=" + mConfig.liveOnly + ")");
        if (!mConfig.enabled) {
            hide();
        } else {
            show();
            filterDisplayMatches();
            EspnFetcher.trigger();
        }
    }

    public static void init(Context context) {
        log("ScoreBoardHud: Initializing Cyber Live Ticker HUD with Logos...");
        mHandler = new Handler(Looper.getMainLooper());
        mConfig = ScoreConfig.load();

        mWindowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);

        mParams = new WindowManager.LayoutParams();
        mParams.width = WindowManager.LayoutParams.MATCH_PARENT;
        mParams.height = 58; // Proportional futuristic HUD height
        mParams.gravity = Gravity.BOTTOM;
        mParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        mParams.format = PixelFormat.TRANSLUCENT;
        mParams.type = WindowManager.LayoutParams.TYPE_PHONE;

        mTickerView = new TickerView(context);
        mWindowManager.addView(mTickerView, mParams);
        log("ScoreBoardHud view added to WindowManager successfully!");

        mTickerView.start();

        // 1. Listen for Broadcast intents
        IntentFilter filter = new IntentFilter();
        filter.addAction("lab.scoreboard.ACTION_SHOW");
        filter.addAction("lab.scoreboard.ACTION_HIDE");
        filter.addAction("lab.scoreboard.ACTION_TOGGLE");
        filter.addAction("lab.scoreboard.ACTION_SPEED");
        filter.addAction("lab.scoreboard.ACTION_RELOAD_CFG");

        try {
            context.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context ctx, Intent intent) {
                    String action = intent.getAction();
                    if ("lab.scoreboard.ACTION_SHOW".equals(action)) {
                        show();
                    } else if ("lab.scoreboard.ACTION_HIDE".equals(action)) {
                        hide();
                    } else if ("lab.scoreboard.ACTION_TOGGLE".equals(action)) {
                        toggle();
                    } else if ("lab.scoreboard.ACTION_RELOAD_CFG".equals(action)) {
                        reloadConfig();
                    } else if ("lab.scoreboard.ACTION_SPEED".equals(action)) {
                        float spd = intent.getFloatExtra("speed", 2.0f);
                        if (mTickerView != null) mTickerView.setSpeed(spd);
                    }
                }
            }, filter);
        } catch (Throwable t) {
            log("BroadcastReceiver registration skipped: " + t.getMessage());
        }

        // 2. Start direct TCP control socket on localhost:8999
        startTcpControlServer();

        // 3. Fallback file watcher on /data/plugin/scoreboard_cmd
        startCommandWatcher();

        // 4. Start Background ESPN Live Engine with Logo Caching
        EspnFetcher.startEngine();
    }

    private static void startTcpControlServer() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ServerSocket ss = new ServerSocket();
                    ss.setReuseAddress(true);
                    ss.bind(new InetSocketAddress("127.0.0.1", 8999));
                    log("ScoreBoardHud: TCP control server listening on 127.0.0.1:8999");
                    while (true) {
                        try {
                            Socket s = ss.accept();
                            BufferedReader br = new BufferedReader(new InputStreamReader(s.getInputStream()));
                            String cmd = br.readLine();
                            if (cmd != null) {
                                cmd = cmd.trim();
                                handleCommand(cmd);
                            }
                            OutputStream out = s.getOutputStream();
                            out.write("OK\n".getBytes());
                            out.flush();
                            s.close();
                        } catch (Exception ex) {
                            log("ScoreBoardHud: TCP client handle error: " + ex.getMessage());
                        }
                    }
                } catch (Exception e) {
                    log("ScoreBoardHud: TCP server bind error: " + e.getMessage());
                }
            }
        }, "ScoreBoardTcpServer");
        t.setDaemon(true);
        t.start();
    }

    private static void startCommandWatcher() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                File f = new File("/data/plugin/scoreboard_cmd");
                long lastModified = (f.exists() && f.length() > 0) ? f.lastModified() : 0;
                while (true) {
                    try {
                        Thread.sleep(250);
                        if (f.exists()) {
                            long lm = f.lastModified();
                            if (lm != lastModified) {
                                lastModified = lm;
                                BufferedReader br = new BufferedReader(new FileReader(f));
                                String cmd = br.readLine();
                                br.close();
                                if (cmd != null) {
                                    handleCommand(cmd.trim());
                                }
                            }
                        }
                    } catch (Exception ignored) {}
                }
            }
        }, "ScoreBoardWatcherThread");
        t.setDaemon(true);
        t.start();
    }

    private static void handleCommand(final String cmd) {
        if (cmd == null || cmd.isEmpty()) return;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                log("Executing command: [" + cmd + "]");
                if ("toggle".equalsIgnoreCase(cmd)) {
                    toggle();
                } else if ("show".equalsIgnoreCase(cmd)) {
                    show();
                } else if ("hide".equalsIgnoreCase(cmd)) {
                    hide();
                } else if ("reload_cfg".equalsIgnoreCase(cmd)) {
                    reloadConfig();
                } else if (cmd.startsWith("speed ")) {
                    try {
                        float spd = Float.parseFloat(cmd.substring(6).trim());
                        if (mTickerView != null) mTickerView.setSpeed(spd);
                    } catch (Exception ignored) {}
                }
            }
        });
    }

    public static class PermissiveTrustManager implements X509TrustManager {
        public X509Certificate[] getAcceptedIssuers() { return new X509Certificate[0]; }
        public void checkClientTrusted(X509Certificate[] certs, String authType) {}
        public void checkServerTrusted(X509Certificate[] certs, String authType) {}
    }

    public static class PermissiveHostnameVerifier implements HostnameVerifier {
        public boolean verify(String hostname, SSLSession session) {
            return true;
        }
    }

    public static void trustAllCertificates() {
        try {
            SSLContext sc = SSLContext.getInstance("TLS");
            sc.init(null, new TrustManager[]{ new PermissiveTrustManager() }, new java.security.SecureRandom());
            HttpsURLConnection.setDefaultSSLSocketFactory(sc.getSocketFactory());
            HttpsURLConnection.setDefaultHostnameVerifier(new PermissiveHostnameVerifier());
        } catch (Exception ignored) {}
    }

    public static class EspnFetcher extends Thread {
        private static final Map<String, String> TEAM_AR = new HashMap<String, String>();

        private static void add(String en, String ar) {
            if (en != null && ar != null) {
                TEAM_AR.put(en.trim().toLowerCase(), ar.trim());
            }
        }

        static {
            // === 1. Premier League ===
            add("Arsenal", "أرسنال");
            add("Aston Villa", "أستون فيلا");
            add("Brentford", "برينتفورد");
            add("Brighton & Hove Albion", "برايتون");
            add("Brighton", "برايتون");
            add("Chelsea", "تشيلسي");
            add("Crystal Palace", "كريستال بالاس");
            add("Everton", "إيفرتون");
            add("Fulham", "فولهام");
            add("Liverpool", "ليفربول");
            add("Manchester City", "مانشستر سيتي");
            add("Man City", "مانشستر سيتي");
            add("Manchester United", "مانشستر يونايتد");
            add("Man United", "مانشستر يونايتد");
            add("Newcastle United", "نيوكاسل");
            add("Newcastle", "نيوكاسل");
            add("Nottingham Forest", "نوتينغهام فورست");
            add("Tottenham Hotspur", "توتنهام");
            add("Tottenham", "توتنهام");
            add("Spurs", "توتنهام");
            add("West Ham United", "وست هام");
            add("West Ham", "وست هام");
            add("Wolverhampton Wanderers", "وولفرهامبتون");
            add("Wolverhampton", "وولفرهامبتون");
            add("Wolves", "وولفرهامبتون");
            add("Leicester City", "ليستر سيتي");
            add("Leicester", "ليستر سيتي");
            add("Southampton", "ساوثهامبتون");
            add("Ipswich Town", "إبسويتش تاون");
            add("Ipswich", "إبسويتش تاون");
            add("Bournemouth", "بورنموث");
            add("AFC Bournemouth", "بورنموث");
            add("Leeds United", "ليدز يونايتد");
            add("Leeds", "ليدز يونايتد");
            add("Burnley", "بيرنلي");
            add("Sheffield United", "شيفيلد يونايتد");
            add("Luton Town", "لوتون تاون");
            add("Sunderland", "سندرلاند");
            add("Middlesbrough", "ميدلزبره");
            add("West Bromwich Albion", "وست بروميتش");
            add("Watford", "واتفورد");
            add("Norwich City", "نورويتش سيتي");
            add("Stoke City", "ستوك سيتي");

            // === 2. La Liga ===
            add("Real Madrid", "ريال مدريد");
            add("Barcelona", "برشلونة");
            add("Barca", "برشلونة");
            add("Atlético Madrid", "أتلتيكو مدريد");
            add("Atletico Madrid", "أتلتيكو مدريد");
            add("Sevilla", "إشبيلية");
            add("Real Sociedad", "ريال سوسيداد");
            add("Athletic Club", "أتلتيك بلباو");
            add("Athletic Bilbao", "أتلتيك بلباو");
            add("Villarreal", "فياريال");
            add("Real Betis", "ريال بيتيس");
            add("Betis", "ريال بيتيس");
            add("Valencia", "فالنسيا");
            add("Girona", "جيرونا");
            add("Mallorca", "مايوركا");
            add("Osasuna", "أوساسونا");
            add("Celta Vigo", "سيلتا فيغو");
            add("Celta de Vigo", "سيلتا فيغو");
            add("Getafe", "خيتافي");
            add("Espanyol", "إسبانيول");
            add("Rayo Vallecano", "رايو فايكانو");
            add("Alavés", "ألافيس");
            add("Alaves", "ألافيس");
            add("Deportivo Alavés", "ألافيس");
            add("Leganés", "ليغانيس");
            add("Leganes", "ليغانيس");
            add("Las Palmas", "لاس بالماس");
            add("UD Las Palmas", "لاس بالماس");
            add("Real Valladolid", "ريال بلد الوليد");
            add("Valladolid", "ريال بلد الوليد");
            add("Granada", "غرناطة");
            add("Cádiz", "قادش");
            add("Cadiz", "قادش");
            add("Almería", "ألميريا");
            add("Almeria", "ألميريا");
            add("Málaga", "مالقا");
            add("Malaga", "مالقا");
            add("Deportivo La Coruña", "ديبورتيفو لاكورونيا");

            // === 3. Serie A ===
            add("Internazionale", "إنتر ميلان");
            add("Inter Milan", "إنتر ميلان");
            add("Inter", "إنتر ميلان");
            add("Juventus", "يوفنتوس");
            add("Juve", "يوفنتوس");
            add("AC Milan", "ميلان");
            add("Milan", "ميلان");
            add("Napoli", "نابولي");
            add("Roma", "روما");
            add("AS Roma", "روما");
            add("Lazio", "لاتسيو");
            add("SS Lazio", "لاتسيو");
            add("Atalanta", "أتالانتا");
            add("Fiorentina", "فيورنتينا");
            add("Torino", "تورينو");
            add("Bologna", "بولونيا");
            add("Genoa", "جنوى");
            add("Parma", "بارما");
            add("Como", "كومو");
            add("Empoli", "إمبولي");
            add("Hellas Verona", "هيلاس فيرونا");
            add("Verona", "هيلاس فيرونا");
            add("Lecce", "ليتشي");
            add("Monza", "مونزا");
            add("Udinese", "أودينيزي");
            add("Cagliari", "كالياري");
            add("Venezia", "فينيسيا");
            add("Sassuolo", "ساسولو");
            add("Salernitana", "ساليرنيتانا");
            add("Frosinone", "فروزينوني");
            add("Sampdoria", "سامبدوريا");
            add("Spezia", "سبيزيا");
            add("Cremonese", "كريمونيزي");

            // === 4. Bundesliga ===
            add("Bayern Munich", "بايرن ميونخ");
            add("Bayern München", "بايرن ميونخ");
            add("Borussia Dortmund", "بوروسيا دورتموند");
            add("Dortmund", "بوروسيا دورتموند");
            add("BVB", "بوروسيا دورتموند");
            add("Bayer Leverkusen", "باير ليفركوزن");
            add("Leverkusen", "باير ليفركوزن");
            add("RB Leipzig", "لايبزيغ");
            add("Leipzig", "لايبزيغ");
            add("Eintracht Frankfurt", "آينتراخت فرانكفورت");
            add("Frankfurt", "فرانكفورت");
            add("VfB Stuttgart", "شتوتغارت");
            add("Stuttgart", "شتوتغارت");
            add("VfL Wolfsburg", "فولفسبورغ");
            add("Wolfsburg", "فولفسبورغ");
            add("Borussia Mönchengladbach", "بوروسيا مونشنغلادباخ");
            add("Mönchengladbach", "مونشنغلادباخ");
            add("Gladbach", "مونشنغلادباخ");
            add("SC Freiburg", "فرايبورغ");
            add("Freiburg", "فرايبورغ");
            add("TSG Hoffenheim", "هوفنهايم");
            add("Hoffenheim", "هوفنهايم");
            add("Werder Bremen", "فيردر بريمن");
            add("Bremen", "فيردر بريمن");
            add("1. FC Union Berlin", "يونيون برلين");
            add("Union Berlin", "يونيون برلين");
            add("FC Augsburg", "أوغسبورغ");
            add("Augsburg", "أوغسبورغ");
            add("1. FSV Mainz 05", "ماينز");
            add("Mainz", "ماينز");
            add("1. FC Heidenheim", "هايدنهايم");
            add("Heidenheim", "هايدنهايم");
            add("FC St. Pauli", "سانت باولي");
            add("St. Pauli", "سانت باولي");
            add("Holstein Kiel", "هولشتاين كيق");
            add("VfL Bochum", "بوخوم");
            add("Bochum", "بوخوم");
            add("1. FC Köln", "كولن");
            add("FC Koln", "كولن");
            add("Hertha BSC", "هيرتا برلين");
            add("Schalke 04", "شالكه");
            add("Hamburger SV", "هامبورغ");

            // === 5. Ligue 1 ===
            add("Paris Saint-Germain", "باريس سان جيرمان");
            add("Paris SG", "باريس سان جيرمان");
            add("PSG", "باريس سان جيرمان");
            add("Monaco", "موناكو");
            add("AS Monaco", "موناكو");
            add("Marseille", "مارسيليا");
            add("Olympique de Marseille", "مارسيليا");
            add("Lyon", "ليون");
            add("Olympique Lyonnais", "ليون");
            add("Lille", "ليل");
            add("LOSC Lille", "ليل");
            add("Lens", "لانس");
            add("RC Lens", "لانس");
            add("Nice", "نيس");
            add("OGC Nice", "نيس");
            add("Rennes", "رين");
            add("Stade Rennais", "رين");
            add("Stade de Reims", "ريمس");
            add("Reims", "ريمس");
            add("Brest", "بريست");
            add("Stade Brestois 29", "بريست");
            add("Strasbourg", "ستراسبورغ");
            add("RC Strasbourg", "ستراسبورغ");
            add("Toulouse", "تولوز");
            add("Montpellier", "مونبلييه");
            add("AJ Auxerre", "أوكسير");
            add("Auxerre", "أوكسير");
            add("Angers", "أنجيه");
            add("Saint-Étienne", "سانت إتيان");
            add("Saint-Etienne", "سانت إتيان");
            add("Le Havre", "لوهافر");
            add("Nantes", "نانت");
            add("FC Nantes", "نانت");
            add("Metz", "ميتز");
            add("Lorient", "لوريان");
            add("Clermont", "كليرمون");
            add("Bordeaux", "بوردو");

            // === 6. European / UCL / UEL / UECL ===
            add("Benfica", "بنفيكا");
            add("SL Benfica", "بنفيكا");
            add("Sporting CP", "سبورتينغ لشبونة");
            add("Sporting Lisbon", "سبورتينغ لشبونة");
            add("Porto", "بورتو");
            add("FC Porto", "بورتو");
            add("Braga", "سبورتينغ براغا");
            add("SC Braga", "سبورتينغ براغا");
            add("Vitória SC", "فيتوريا غيمارايش");
            add("Vitoria de Guimaraes", "فيتوريا غيمارايش");
            add("Ajax", "أياكس");
            add("Ajax Amsterdam", "أياكس");
            add("PSV Eindhoven", "آيندهوفن");
            add("PSV", "آيندهوفن");
            add("Feyenoord", "فينورد");
            add("AZ Alkmaar", "ألكمار");
            add("FC Twente", "تفينتي");
            add("Celtic", "سيلتيك");
            add("Rangers", "رينجرز");
            add("Heart of Midlothian", "هارتس");
            add("Galatasaray", "غلطة سراي");
            add("Fenerbahce", "فنربخشة");
            add("Fenerbahçe", "فنربخشة");
            add("Besiktas", "بشكتاش");
            add("Beşiktaş", "بشكتاش");
            add("Trabzonspor", "طرابزون سبور");
            add("Basaksehir", "باشاك شهير");
            add("Club Brugge", "كلوب بروج");
            add("Anderlecht", "أندرلخت");
            add("RSC Anderlecht", "أندرلخت");
            add("KAA Gent", "خنت");
            add("Gent", "خنت");
            add("Genk", "جينك");
            add("KRC Genk", "جينك");
            add("Royal Antwerp", "أنتويرب");
            add("Union Saint-Gilloise", "سانت جيلواز");
            add("Union St.-Gilloise", "سانت جيلواز");
            add("Standard Liège", "ستاندارد لييج");
            add("Young Boys", "يونغ بويز");
            add("BSC Young Boys", "يونغ بويز");
            add("FC Basel", "بازل");
            add("FC Zurich", "زيورخ");
            add("Servette", "سيرفيت");
            add("Lugano", "لوغانو");
            add("FC Lugano", "لوغانو");
            add("Red Bull Salzburg", "سالزبورغ");
            add("RB Salzburg", "سالزبورغ");
            add("Sturm Graz", "شتورم غراتس");
            add("SK Sturm Graz", "شتورم غراتس");
            add("Rapid Wien", "رابيد فيينا");
            add("LASK", "لاسك لينز");
            add("Shakhtar Donetsk", "شاختار دونيتسك");
            add("Dynamo Kyiv", "دينامو كييف");
            add("Slavia Prague", "سلافيا براغ");
            add("Slavia Praha", "سلافيا براغ");
            add("Sparta Prague", "سبارتا براغ");
            add("Sparta Praha", "سبارتا براغ");
            add("Viktoria Plzen", "فيكتوريا بلزن");
            add("Viktoria Plzeň", "فيكتوريا بلزن");
            add("Dinamo Zagreb", "دينامو زغرب");
            add("Hajduk Split", "هايدوك سبليت");
            add("Crvena Zvezda", "النجم الأحمر");
            add("Red Star Belgrade", "النجم الأحمر");
            add("Partizan", "بارتيزان بلغراد");
            add("Olympiacos", "أولمبياكوس");
            add("Panathinaikos", "باناثينايكوس");
            add("AEK Athens", "أيك أثينا");
            add("PAOK", "باوك سالونيكا");
            add("Aris", "أريس سالونيكا");
            add("Copenhagen", "كوبنهاغن");
            add("FC Copenhagen", "كوبنهاغن");
            add("F.C. København", "كوبنهاغن");
            add("Midtjylland", "ميتييلاند");
            add("FC Midtjylland", "ميتييلاند");
            add("Brøndby", "بروندبي");
            add("Bodø/Glimt", "بودو/غليمت");
            add("Bodo/Glimt", "بودو/غليمت");
            add("Molde", "مولده");
            add("Rosenborg", "روزنبورغ");
            add("Malmö FF", "مالمو");
            add("Malmo FF", "مالمو");
            add("Djurgården", "يورغوردين");
            add("Ferencváros", "فيرينتسفاروش");
            add("Ferencvaros", "فيرينتسفاروش");
            add("Legia Warsaw", "ليغيا وارسو");
            add("Lech Poznan", "ليخ بوزنان");
            add("Jagiellonia Bialystok", "ياغيلونيا");
            add("Qarabağ", "قره باغ");
            add("Qarabag", "قره باغ");
            add("Sabah FK", "صباح");
            add("Sabah", "صباح");
            add("Sheriff Tiraspol", "شريف تيراسبول");
            add("Maccabi Tel Aviv", "مكابي تل أبيب");
            add("Maccabi Haifa", "مكابي حيفا");
            add("Ludogorets Razgrad", "لودوغوريتس");
            add("CSKA Sofia", "سسكا صوفيا");
            add("Levski Sofia", "ليفسكي صوفيا");
            add("APOEL", "أبويل نيقوسيا");
            add("Omonia", "أومونيا");
            add("Pafos", "بافوس");
            add("Kairat Almaty", "كايرات");
            add("Astana", "أستانا");
            add("Slovan Bratislava", "سلوفان براتيسلافا");

            // === 7. Saudi Pro League ===
            add("Al Hilal", "الهلال");
            add("Al-Hilal", "الهلال");
            add("Al Nassr", "النصر");
            add("Al-Nassr", "النصر");
            add("Al Ittihad", "الاتحاد");
            add("Al-Ittihad", "الاتحاد");
            add("Al Ahli", "الأهلي السعودي");
            add("Al-Ahli", "الأهلي السعودي");
            add("Al Shabab", "الشباب");
            add("Al-Shabab", "الشباب");
            add("Al Ettifaq", "الاتفاق");
            add("Al-Ettifaq", "الاتفاق");
            add("Al Fateh", "الفتح");
            add("Al-Fateh", "الفتح");
            add("Al Taawoun", "التعاون");
            add("Al-Taawoun", "التعاون");
            add("Al Qadsiah", "القادسية");
            add("Al-Qadsiah", "القادسية");
            add("Al Wehda", "الوحدة");
            add("Al-Wehda", "الوحدة");
            add("Al Fayha", "الفيحاء");
            add("Al-Fayha", "الفيحاء");
            add("Damac", "ضمك");
            add("Al Raed", "الرائد");
            add("Al-Raed", "الرائد");
            add("Al Kholood", "الخلود");
            add("Al-Kholood", "الخلود");
            add("Al Orobah", "العروبة");
            add("Al-Orobah", "العروبة");
            add("Al Riyadh", "الرياض");
            add("Al-Riyadh", "الرياض");
            add("Al Akhdoud", "الأخدود");
            add("Al-Akhdoud", "الأخدود");
            add("Al Hazem", "الحزم");
            add("Al Tai", "الطائي");
            add("Al Batin", "الباطن");
            add("Abha", "أبها");
            add("Al Diriyah", "الدرعية");

            // === 8. Egyptian & North African & Arab ===
            add("Al Ahly", "الأهلي المصري");
            add("Al-Ahly", "الأهلي المصري");
            add("Zamalek", "الزمالك");
            add("Pyramids", "بيراميدز");
            add("Pyramids FC", "بيراميدز");
            add("Al Masry", "المصري البورسعيدي");
            add("Ismaily", "الإسماعيلي");
            add("Modern Sport", "مودرن سبورت");
            add("Future FC", "مودرن سبورت");
            add("Smouha", "سموحة");
            add("ZED FC", "زد");
            add("Ceramica Cleopatra", "سيراميكا كليوباترا");
            add("ENPPI", "إنبي");
            add("Al Ittihad Alexandria", "الاتحاد السكندري");
            add("Tala'ea El Gaish", "طلائع الجيش");
            add("National Bank", "البنك الأهلي");

            add("Esperance", "الترجي التونسي");
            add("Esperance de Tunis", "الترجي التونسي");
            add("ES Tunis", "الترجي التونسي");
            add("Club Africain", "النادي الإفريقي");
            add("Etoile du Sahel", "النجم الساحلي");
            add("CS Sfaxien", "الصفاقسي");
            add("US Monastir", "الاتحاد المنستيري");
            add("Stade Tunisien", "الملعب التونسي");

            add("Wydad", "الوداد البيضاوي");
            add("Wydad Casablanca", "الوداد البيضاوي");
            add("WAC", "الوداد البيضاوي");
            add("Raja", "الرجاء البيضاوي");
            add("Raja Casablanca", "الرجاء البيضاوي");
            add("RCA", "الرجاء البيضاوي");
            add("AS FAR", "الجيش الملكي");
            add("FAR Rabat", "الجيش الملكي");
            add("RS Berkane", "نهضة بركان");
            add("FUS Rabat", "الفتح الرباطي");
            add("MAS Fez", "المغرب الفاسي");
            add("IR Tanger", "اتحاد طنجة");
            add("Moghreb Tetouan", "المغرب التطواني");
            add("Olympic Safi", "أولمبيك آسفي");
            add("Hassania Agadir", "حسنية أكادير");

            add("MC Alger", "مولودية الجزائر");
            add("MCA", "مولودية الجزائر");
            add("CR Belouizdad", "شباب بلوزداد");
            add("CRB", "شباب بلوزداد");
            add("JS Kabylie", "شبيبة القبائل");
            add("JSK", "شبيبة القبائل");
            add("USM Alger", "اتحاد العاصمة");
            add("USMA", "اتحاد العاصمة");
            add("ES Setif", "وفاق سطيف");
            add("ESS", "وفاق سطيف");
            add("CS Constantine", "شباب قسنطينة");
            add("MC Oran", "مولودية وهران");
            add("Paradou AC", "نادي بارادو");
            add("JS Saoura", "شبيبة الساورة");

            add("Al Sadd", "السد القطري");
            add("Al Rayyan", "الريان القطري");
            add("Al Duhail", "الدحيل القطري");
            add("Al Gharafa", "الغرافة القطري");
            add("Al Arabi", "العربي القطري");
            add("Al Wakrah", "الوكرة القطري");
            add("Qatar SC", "نادي قطر");
            add("Al Shamal", "الشمال القطري");

            add("Al Ain", "العين الإماراتي");
            add("Al Wasl", "الوصل الإماراتي");
            add("Al Sharjah", "الشارقة الإماراتي");
            add("Shabab Al Ahli", "شباب الأهلي دبي");
            add("Al Wahda", "الوحدة الإماراتي");
            add("Al Jazira", "الجزيرة الإماراتي");
            add("Al Nasr", "النصر الإماراتي");

            add("Al Shorta", "الشرطة العراقي");
            add("Al Quwa Al Jawiya", "القوة الجوية");
            add("Al-Quwa Al-Jawiya", "القوة الجوية");
            add("Al Zawraa", "الزوراء العراقي");
            add("Al Talaba", "الطلبة العراقي");
            add("Erbil", "أربيل العراقي");

            add("Al Hilal Omdurman", "الهلال السوداني");
            add("Al Merrikh", "المريخ السوداني");
            add("Al Faisaly", "الفيصلي الأردني");
            add("Al Wehdat", "الوحدات الأردني");
            add("Kuwait SC", "نادي الكويت");
            add("Al Qadsia", "القادسية الكويتي");
            add("Al Arabi Kuwait", "العربي الكويتي");
            add("Persepolis", "بيرسبوليس");
            add("Esteghlal", "استقلال طهران");
            add("Sepahan", "سباهان أصفهان");
            add("Traktor Sazi", "تراكتور سازي");
            add("Pakhtakor", "باختاكور");
            add("Pakhtakor Tashkent", "باختاكور");
            add("Neftchi Fergana", "نيفتشي");
            add("Mamelodi Sundowns", "ماميلودي صنداونز");
            add("Sundowns", "صنداونز");
            add("TP Mazembe", "تي بي مازيمبي");
            add("Simba", "سيمبا التنزاني");
            add("Young Africans", "يانغ أفريكانز");
        }

        private static String normalize(String s) {
            if (s == null) return "";
            String r = s.toLowerCase();
            // Remove accents & diacritics
            r = r.replace("á", "a").replace("à", "a").replace("â", "a").replace("ä", "a").replace("ã", "a").replace("å", "a")
                 .replace("é", "e").replace("è", "e").replace("ê", "e").replace("ë", "e")
                 .replace("í", "i").replace("ì", "i").replace("î", "i").replace("ï", "i")
                 .replace("ó", "o").replace("ò", "o").replace("ô", "o").replace("ö", "o").replace("ø", "o").replace("õ", "o")
                 .replace("ú", "u").replace("ù", "u").replace("û", "u").replace("ü", "u")
                 .replace("ñ", "n").replace("ç", "c").replace("š", "s").replace("ž", "z").replace("č", "c")
                 .replace("ć", "c").replace("đ", "d").replace("ß", "ss");
            // Remove special punctuation
            r = r.replace("-", " ").replace("_", " ").replace(".", " ").replace("&", "and");
            r = r.replaceAll("\\s+", " ").trim();
            return r;
        }

        public static String translate(String... names) {
            if (names == null || names.length == 0) return "";
            for (String raw : names) {
                if (raw == null || raw.trim().isEmpty()) continue;
                String clean = raw.trim();
                String lower = clean.toLowerCase();
                String norm = normalize(clean);

                // 1. Direct dictionary match
                if (TEAM_AR.containsKey(lower)) return TEAM_AR.get(lower);
                if (TEAM_AR.containsKey(norm)) return TEAM_AR.get(norm);

                // 2. Stripped prefix / suffix match
                String stripped = norm.replaceAll("(?i)\\b(fc|cf|sc|ac|afc|fk|sk|cd|ud|rcd|ogc|losc|sv|vfb|vfl|tsg|as|us|ss|club)\\b", "")
                                      .replaceAll("\\s+", " ").trim();
                if (TEAM_AR.containsKey(stripped)) return TEAM_AR.get(stripped);

                // Strip 'al ' prefix
                if (stripped.startsWith("al ")) {
                    String withoutAl = stripped.substring(3).trim();
                    if (TEAM_AR.containsKey(withoutAl)) return TEAM_AR.get(withoutAl);
                    if (TEAM_AR.containsKey("al " + withoutAl)) return TEAM_AR.get("al " + withoutAl);
                }

                // 3. Keyword / partial matching
                for (Map.Entry<String, String> e : TEAM_AR.entrySet()) {
                    String k = e.getKey();
                    if (k.length() >= 5 && (norm.contains(k) || k.contains(norm))) {
                        return e.getValue();
                    }
                }
            }

            // Fallback to first non-empty raw name
            for (String raw : names) {
                if (raw != null && !raw.trim().isEmpty()) return raw.trim();
            }
            return "";
        }

        private static volatile boolean sTriggerNow = false;
        private static EspnFetcher sThread = null;

        public static synchronized void startEngine() {
            if (sThread != null && sThread.isAlive()) return;
            sThread = new EspnFetcher();
            sThread.setName("EspnFetcherThread");
            sThread.setDaemon(true);
            sThread.start();
        }

        public static void trigger() {
            sTriggerNow = true;
            if (sThread != null) {
                sThread.interrupt();
            }
        }

        @Override
        public void run() {
            trustAllCertificates();
            log("EspnFetcher: Started background live data engine with Logo support");

            while (true) {
                try {
                    fetchAndUpdate();
                } catch (Throwable t) {
                    log("EspnFetcher fetch error: " + t.getMessage());
                }

                try {
                    sTriggerNow = false;
                    for (int s = 0; s < 60; s++) {
                        if (sTriggerNow) break;
                        Thread.sleep(1000);
                    }
                } catch (InterruptedException ignored) {}
            }
        }

        private void fetchAndUpdate() {
            List<MatchItem> fresh = new ArrayList<MatchItem>();

            // 1. UCL & UEL
            if (mConfig.showUcl) {
                fetchLeague("ucl", "uefa.champions", "دوري أبطال أوروبا", fresh);
                fetchLeague("ucl", "uefa.europa", "الدوري الأوروبي", fresh);
                fetchLeague("ucl", "uefa.europa.conf", "دوري المؤتمر الأوروبي", fresh);
            }
            // 2. EPL
            if (mConfig.showEpl) {
                fetchLeague("epl", "eng.1", "الدوري الإنجليزي", fresh);
            }
            // 3. La Liga
            if (mConfig.showLaliga) {
                fetchLeague("laliga", "esp.1", "الدوري الإسباني", fresh);
            }
            // 4. Serie A
            if (mConfig.showSeriea) {
                fetchLeague("seriea", "ita.1", "الدوري الإيطالي", fresh);
                fetchLeague("seriea", "ger.1", "الدوري الألماني", fresh);
                fetchLeague("seriea", "fra.1", "الدوري الفرنسي", fresh);
            }
            // 5. Arab & Local
            if (mConfig.showArab) {
                fetchLeague("arab", "ksa.1", "دوري روشن السعودي", fresh);
                fetchLeague("arab", "caf.champions", "دوري أبطال أفريقيا", fresh);
                fetchLeague("arab", "afc.champions", "دوري أبطال آسيا", fresh);
            }

            if (!fresh.isEmpty()) {
                // Ensure logos are pre-downloaded and attached
                for (MatchItem m : fresh) {
                    downloadOneLogo(m.logo1Url);
                    downloadOneLogo(m.logo2Url);
                    m.logo1 = getLogo(m.logo1Url);
                    m.logo2 = getLogo(m.logo2Url);
                }

                synchronized (mAllMatches) {
                    mAllMatches.clear();
                    mAllMatches.addAll(fresh);
                }
                filterDisplayMatches();
                if (mTickerView != null) {
                    mTickerView.postInvalidate();
                }
                log("EspnFetcher: Successfully updated " + fresh.size() + " matches live from ESPN with logos!");
            }
        }

        private void fetchLeague(String lKey, String espnCode, String defaultName, List<MatchItem> outList) {
            HttpURLConnection conn = null;
            try {
                URL u = new URL("https://site.api.espn.com/apis/site/v2/sports/soccer/" + espnCode + "/scoreboard");
                conn = (HttpURLConnection) u.openConnection();
                conn.setConnectTimeout(6000);
                conn.setReadTimeout(8000);
                conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
                conn.connect();

                if (conn.getResponseCode() != 200) {
                    log("EspnFetcher: " + espnCode + " returned HTTP " + conn.getResponseCode());
                    return;
                }

                BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line);
                }
                reader.close();

                JSONObject root = new JSONObject(sb.toString());
                String compName = defaultName;
                if (root.has("leagues")) {
                    JSONArray lgArr = root.getJSONArray("leagues");
                    if (lgArr.length() > 0) {
                        JSONObject lgObj = lgArr.getJSONObject(0);
                        if (lgObj.has("name") && defaultName.isEmpty()) {
                            compName = lgObj.getString("name");
                        }
                    }
                }

                if (!root.has("events")) return;
                JSONArray events = root.getJSONArray("events");

                for (int i = 0; i < events.length(); i++) {
                    JSONObject ev = events.getJSONObject(i);
                    JSONObject status = ev.optJSONObject("status");
                    String sType = "";
                    String clock = "";
                    String shortDetail = "مجدولة";
                    int period = 0;
                    boolean isCompleted = false;
                    String state = "";
                    if (status != null) {
                        clock = status.optString("displayClock", "");
                        period = status.optInt("period", 0);
                        JSONObject typeObj = status.optJSONObject("type");
                        if (typeObj != null) {
                            sType = typeObj.optString("name", "");
                            shortDetail = typeObj.optString("shortDetail", "مجدولة");
                            isCompleted = typeObj.optBoolean("completed", false);
                            state = typeObj.optString("state", "");
                        }
                    }

                    boolean isLive = false;
                    String statusText = "مجدولة";
                    if (sType.contains("POSTPONED")) {
                        statusText = "مؤجلة";
                        isLive = false;
                    } else if (sType.contains("ABANDONED")) {
                        statusText = "ملغاة (ABN)";
                        isLive = false;
                    } else if (sType.contains("CANCELED") || sType.contains("CANCELLED")) {
                        statusText = "ملغاة";
                        isLive = false;
                    } else if (isCompleted || "post".equalsIgnoreCase(state) || sType.contains("FINAL") || sType.contains("FULL_TIME")) {
                        if (sType.contains("SHOOTOUT") || sType.contains("PENALTIES") || period == 5) {
                            statusText = "انتهت (ركلات ترجيح)";
                        } else if (sType.contains("EXTRA_TIME") || sType.contains("OVERTIME") || period == 3 || period == 4) {
                            statusText = "انتهت (وقت إضافي)";
                        } else {
                            statusText = "انتهت";
                        }
                        isLive = false;
                    } else if (sType.contains("HALFTIME") || sType.contains("HALF_TIME")) {
                        statusText = "استراحة (HT)";
                        isLive = true;
                    } else if (sType.contains("SHOOTOUT") || sType.contains("PENALTIES") || period == 5) {
                        statusText = "ركلات ترجيح";
                        isLive = true;
                    } else if (sType.contains("EXTRA_TIME") || sType.contains("OVERTIME") || period == 3 || period == 4) {
                        String pLabel = (period == 4) ? "إضافي 2" : "إضافي 1";
                        statusText = clock.isEmpty() ? pLabel : (pLabel + " (" + clock + ")");
                        isLive = true;
                    } else if (sType.contains("SECOND_HALF") || period == 2) {
                        statusText = "الشوط 2 (" + (clock.isEmpty() ? "45'" : clock) + ")";
                        isLive = true;
                    } else if (sType.contains("FIRST_HALF") || period == 1 || sType.contains("IN_PROGRESS") || "in".equalsIgnoreCase(state)) {
                        statusText = "الشوط 1 (" + (clock.isEmpty() ? "1'" : clock) + ")";
                        isLive = true;
                    } else {
                        if ("Scheduled".equalsIgnoreCase(shortDetail)) {
                            statusText = "مجدولة";
                        } else {
                            statusText = shortDetail;
                        }
                        isLive = false;
                    }

                    JSONArray comps = ev.optJSONArray("competitions");
                    if (comps == null || comps.length() == 0) continue;
                    JSONObject comp = comps.getJSONObject(0);

                    JSONArray competitors = comp.optJSONArray("competitors");
                    if (competitors == null || competitors.length() < 2) continue;

                    JSONObject home = null;
                    JSONObject away = null;
                    for (int c = 0; c < competitors.length(); c++) {
                        JSONObject cand = competitors.getJSONObject(c);
                        if ("home".equalsIgnoreCase(cand.optString("homeAway"))) {
                            home = cand;
                        } else {
                            away = cand;
                        }
                    }
                    if (home == null) home = competitors.getJSONObject(0);
                    if (away == null) away = competitors.getJSONObject(1);

                    JSONObject t1Obj = home.optJSONObject("team");
                    JSONObject t2Obj = away.optJSONObject("team");

                    String t1Raw = (t1Obj != null) ? t1Obj.optString("displayName", "فريق 1") : "فريق 1";
                    String t1Name = (t1Obj != null) ? t1Obj.optString("name", "") : "";
                    String t1Short = (t1Obj != null) ? t1Obj.optString("shortDisplayName", "") : "";
                    String logo1Url = (t1Obj != null) ? t1Obj.optString("logo", "") : "";

                    String t2Raw = (t2Obj != null) ? t2Obj.optString("displayName", "فريق 2") : "فريق 2";
                    String t2Name = (t2Obj != null) ? t2Obj.optString("name", "") : "";
                    String t2Short = (t2Obj != null) ? t2Obj.optString("shortDisplayName", "") : "";
                    String logo2Url = (t2Obj != null) ? t2Obj.optString("logo", "") : "";

                    String t1 = translate(t1Raw, t1Name, t1Short);
                    String t2 = translate(t2Raw, t2Name, t2Short);

                    int s1 = 0;
                    int s2 = 0;
                    try { s1 = Integer.parseInt(home.optString("score", "0")); } catch (Exception ignored) {}
                    try { s2 = Integer.parseInt(away.optString("score", "0")); } catch (Exception ignored) {}

                    List<String> scorersList = new ArrayList<String>();
                    List<String> yellowsList = new ArrayList<String>();
                    List<String> redsList = new ArrayList<String>();

                    JSONArray details = comp.optJSONArray("details");
                    if (details != null) {
                        for (int d = 0; d < details.length(); d++) {
                            JSONObject det = details.getJSONObject(d);
                            String dType = det.optJSONObject("type") != null ? det.getJSONObject("type").optString("text", "") : "";
                            String dClock = det.optJSONObject("clock") != null ? det.getJSONObject("clock").optString("displayValue", "") : "";
                            String athName = "";
                            JSONArray aths = det.optJSONArray("athletesInvolved");
                            if (aths != null && aths.length() > 0) {
                                JSONObject ath = aths.getJSONObject(0);
                                athName = ath.optString("shortName", ath.optString("displayName", ""));
                            }

                            if (det.optBoolean("scoringPlay", false) || "Goal".equalsIgnoreCase(dType)) {
                                if (!athName.isEmpty()) scorersList.add(athName + " " + dClock);
                            } else if (det.optBoolean("yellowCard", false)) {
                                if (!athName.isEmpty()) yellowsList.add(athName + " " + dClock);
                            } else if (det.optBoolean("redCard", false)) {
                                if (!athName.isEmpty()) redsList.add(athName + " " + dClock + " (طرد)");
                            }
                        }
                    }

                    String scorers = joinStrings(scorersList, " ، ");
                    String yellows = joinStrings(yellowsList, " ، ");
                    String reds = joinStrings(redsList, " ، ");

                    outList.add(new MatchItem(lKey, compName, t1, s1, s2, t2, statusText, isLive, scorers, yellows, reds, logo1Url, logo2Url));
                }
            } catch (Exception e) {
                log("EspnFetcher error for " + espnCode + ": " + e.getMessage());
            } finally {
                if (conn != null) conn.disconnect();
            }
        }

        private static String joinStrings(List<String> list, String delim) {
            if (list == null || list.isEmpty()) return "";
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < list.size(); i++) {
                if (i > 0) sb.append(delim);
                sb.append(list.get(i));
            }
            return sb.toString();
        }
    }

    public static void log(String msg) {
        System.out.println("[ScoreBoardHud] " + msg);
        try {
            File logFile = new File("/data/plugin/ColorPro_data/scoreboard.log");
            FileWriter fw = new FileWriter(logFile, true);
            fw.write(new java.util.Date() + " " + msg + "\n");
            fw.close();
        } catch (Exception ignored) {}
    }

    public static void main(String[] args) {
        log("ScoreBoardHud entry point invoked directly");
        try {
            Looper.prepareMainLooper();
            Class<?> atCls = Class.forName("android.app.ActivityThread");
            Method curAtMeth = atCls.getMethod("currentActivityThread");
            Object at = curAtMeth.invoke(null);
            if (at == null) {
                Method initMeth = atCls.getMethod("systemMain");
                at = initMeth.invoke(null);
            }
            Method getSysCtx = atCls.getMethod("getSystemContext");
            Context ctx = (Context) getSysCtx.invoke(at);

            init(ctx);
            Looper.loop();
        } catch (Exception e) {
            log("ScoreBoardHud fatal startup error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
