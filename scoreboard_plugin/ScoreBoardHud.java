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

        public MatchItem(String leagueKey, String comp, String t1, int s1, int s2, String t2,
                         String statusText, boolean live, String scorers, String yellowCards, String redCards) {
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
        }
    }

    private static final List<MatchItem> mAllMatches = new ArrayList<MatchItem>();
    private static final List<MatchItem> mDisplayMatches = Collections.synchronizedList(new ArrayList<MatchItem>());

    public static class ScrollTask implements Runnable {
        private final TickerView mView;
        public ScrollTask(TickerView view) {
            this.mView = view;
        }
        @Override
        public void run() {
            if (!mView.mRunning) return;
            mView.mScrollX += mView.mSpeed;
            if (mView.mPeriod > 0) {
                if (mView.mScrollX >= mView.mPeriod) {
                    mView.mScrollX -= mView.mPeriod;
                }
            }
            mView.invalidate();
            mView.postDelayed(this, 16);
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
                    String sText = m.isLive ? ("● " + m.statusText) : m.statusText;
                    w += mPaintStatus.measureText(sText) + 12 + 16;
                    w += mPaintText.measureText(m.team1) + 12;
                    String scText = " " + m.score1 + " - " + m.score2 + " ";
                    w += mPaintScore.measureText(scText) + 12 + 14;
                    w += mPaintText.measureText(m.team2) + 16;
                    if (mConfig.showDetails && m.scorers != null && m.scorers.length() > 0) {
                        w += 44 + 8 + mPaintScorers.measureText(m.scorers) + 16;
                    }
                    if (mConfig.showDetails && m.yellowCards != null && m.yellowCards.length() > 0) {
                        w += 10 + 5 + mPaintYellowText.measureText(m.yellowCards) + 16;
                    }
                    if (mConfig.showDetails && m.redCards != null && m.redCards.length() > 0) {
                        w += 10 + 5 + mPaintRedText.measureText(m.redCards) + 16;
                    }
                    w += mPaintBadge.measureText("◈") + 28;
                }
            }
            return (w > 0) ? w : 1920f;
        }

        private void drawMatchTrain(Canvas canvas, float headX, int width, int height, float baseline) {
            if (headX < 0) return;
            float curX = headX;
            synchronized (mDisplayMatches) {
                for (int i = 0; i < mDisplayMatches.size(); i++) {
                    if (curX < -150) break;
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
                    String sText = m.isLive ? ("● " + m.statusText) : m.statusText;
                    int sCol = m.isLive ? Color.parseColor("#00E676") :
                            (m.statusText.contains("FT") ? Color.parseColor("#80D8FF") :
                            (m.statusText.contains("HT") ? Color.parseColor("#FFD700") : Color.parseColor("#B0BEC5")));
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

                    // 3. Team 1
                    float t1W = mPaintText.measureText(m.team1);
                    float t1Left = curX - t1W;
                    if (curX >= 0 && t1Left <= width) {
                        canvas.drawText(m.team1, t1Left, baseline, mPaintText);
                    }
                    curX = t1Left - 12;

                    // 4. Score
                    String scText = " " + m.score1 + " - " + m.score2 + " ";
                    float scTextW = mPaintScore.measureText(scText);
                    float scBoxW = scTextW + 12;
                    float scLeft = curX - scBoxW;
                    if (curX >= 0 && scLeft <= width) {
                        RectF r = new RectF(scLeft, 10, curX, height - 10);
                        canvas.drawRoundRect(r, 6, 6, mPaintScoreBg);
                        canvas.drawRoundRect(r, 6, 6, mPaintScoreBorder);
                        canvas.drawText(scText, scLeft + 6, baseline, mPaintScore);
                    }
                    curX = scLeft - 14;

                    // 5. Team 2
                    float t2W = mPaintText.measureText(m.team2);
                    float t2Left = curX - t2W;
                    if (curX >= 0 && t2Left <= width) {
                        canvas.drawText(m.team2, t2Left, baseline, mPaintText);
                    }
                    curX = t2Left - 16;

                    // 6. Scorers
                    if (mConfig.showDetails && m.scorers != null && m.scorers.length() > 0) {
                        float scW = mPaintScorers.measureText(m.scorers);
                        float totalScW = 44 + 8 + scW;
                        float scL = curX - totalScW;
                        if (curX >= 0 && scL <= width) {
                            RectF goalBadge = new RectF(curX - 44, 14, curX, height - 14);
                            canvas.drawRoundRect(goalBadge, 5, 5, mPaintGoalBadge);
                            canvas.drawText("هدف", curX - 36, baseline - 2, mPaintGoalText);
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
                    float sepW = mPaintBadge.measureText("◈");
                    float sepL = curX - sepW;
                    if (curX >= 0 && sepL <= width) {
                        mPaintBadge.setColor(Color.parseColor("#00E5FF"));
                        canvas.drawText("◈", sepL, baseline, mPaintBadge);
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

            // 1. Futuristic Matte Obsidian Glass Background (ColorPro HUD style)
            mPaintBg.setColor(Color.parseColor("#F2080C14"));
            canvas.drawRect(0, 0, width, height, mPaintBg);

            // 2. Faint Cyber Dot Matrix Grid
            for (float gy = 10; gy < height - 6; gy += 14) {
                for (float gx = 8; gx < width - 8; gx += 18) {
                    canvas.drawCircle(gx, gy, 1.0f, mPaintGrid);
                }
            }

            // 3. Glowing Cyan Top Cyber Bar
            canvas.drawLine(0, 2f, width, 2f, mPaintLine);

            // 4. Slanted Tech Stripes (///) on left and right borders
            for (float hx = 12; hx <= 60; hx += 7) {
                canvas.drawLine(hx, 10, hx + 5, 2.5f, mPaintStripe);
            }
            for (float hx = width - 65; hx <= width - 17; hx += 7) {
                canvas.drawLine(hx, 10, hx + 5, 2.5f, mPaintStripe);
            }

            // 5. Glow dots at top corners
            drawGlowDot(canvas, 7, 2.5f, Color.parseColor("#00E5FF"));
            drawGlowDot(canvas, width - 7, 2.5f, Color.parseColor("#00E5FF"));

            // 6. Subtle bottom hairline
            mPaintStripe.setColor(Color.parseColor("#2200E5FF"));
            canvas.drawLine(0, height - 1f, width, height - 1f, mPaintStripe);

            if (mNeedMeasure || mPeriod <= 0) {
                mPeriod = measureTotalWidth() + 60f;
                mNeedMeasure = false;
            }

            float baseline = height * 0.64f;

            synchronized (mDisplayMatches) {
                if (mDisplayMatches.isEmpty()) {
                    String emptyText = "★ لا توجد مباريات جارية حالياً وفق الخيارات المحددة ★";
                    canvas.drawText(emptyText, width / 2f - mPaintText.measureText(emptyText) / 2f, baseline, mPaintText);
                    return;
                }
            }

            float p = mPeriod;
            float startHead = (p > 0) ? (mScrollX % p) : mScrollX;
            if (startHead < 0) startHead += p;

            float curHead = startHead;
            while (curHead < width + p) {
                drawMatchTrain(canvas, curHead, width, height, baseline);
                curHead += p;
            }
            curHead = startHead - p;
            while (curHead > -p) {
                drawMatchTrain(canvas, curHead, width, height, baseline);
                curHead -= p;
            }
        }
    }

    public static class ScoreBoardReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context ctx, Intent intent) {
            handleIntent(intent);
        }
    }

    public static class VisTask implements Runnable {
        private final boolean mVisible;
        public VisTask(boolean v) {
            this.mVisible = v;
        }
        @Override
        public void run() {
            if (mTickerView == null) return;
            mIsVisible = mVisible;
            if (mVisible) {
                mTickerView.setVisibility(View.VISIBLE);
                mTickerView.start();
                log("ScoreBoardHud: SHOWN");
            } else {
                mTickerView.setVisibility(View.GONE);
                mTickerView.stop();
                log("ScoreBoardHud: HIDDEN");
            }
        }
    }

    public static void main(String[] args) {
        try {
            log("ScoreBoardHud starting...");
            Looper.prepareMainLooper();
            mHandler = new Handler(Looper.myLooper());

            Class<?> atClass = Class.forName("android.app.ActivityThread");
            Object at = atClass.getMethod("systemMain").invoke(null);
            Context context = (Context) atClass.getMethod("getSystemContext").invoke(at);

            mWindowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);

            mParams = new WindowManager.LayoutParams();
            mParams.width = WindowManager.LayoutParams.MATCH_PARENT;
            mParams.height = 58; // Proportional futuristic HUD height
            mParams.type = 2006; // TYPE_KEYGUARD
            mParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                    | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
            mParams.format = PixelFormat.TRANSLUCENT;
            mParams.gravity = Gravity.BOTTOM;
            mParams.x = 0;
            mParams.y = 0;

            mConfig = ScoreConfig.load();
            populateMasterMatches();
            filterDisplayMatches();

            mTickerView = new TickerView(context);
            mWindowManager.addView(mTickerView, mParams);
            mTickerView.start();
            log("ScoreBoardHud view added to WindowManager successfully!");

            registerSystemBroadcast(at, context);
            startCommandServer();
            EspnFetcher.startEngine();

            Looper.loop();
        } catch (Throwable t) {
            log("FATAL ERROR: " + t.getMessage());
            t.printStackTrace();
        }
    }

    private static void registerSystemBroadcast(Object at, Context context) {
        try {
            IntentFilter filter = new IntentFilter();
            filter.addAction("com.android.dvb.SHOW_SCOREBOARD");
            filter.addAction("com.android.dvb.SCORE_BOARD_SHOW");
            filter.addAction("android.intent.action.SCOREBOARD");

            ScoreBoardReceiver receiver = new ScoreBoardReceiver();
            try {
                Method mReg = context.getClass().getMethod("registerReceiver", BroadcastReceiver.class, IntentFilter.class);
                mReg.invoke(context, receiver, filter);
                log("System BroadcastReceiver registered successfully via Context!");
                return;
            } catch (Throwable ignored) {}

            Field fAppOps = at.getClass().getDeclaredField("mAppOps");
            fAppOps.setAccessible(true);
            Object appOps = fAppOps.get(at);

            Class<?> amClass = Class.forName("android.app.ActivityManagerNative");
            Method getDefault = amClass.getMethod("getDefault");
            Object am = getDefault.invoke(null);

            Method[] methods = am.getClass().getMethods();
            for (Method m : methods) {
                if (m.getName().equals("registerReceiver")) {
                    Class<?>[] pTypes = m.getParameterTypes();
                    Object[] params = new Object[pTypes.length];
                    for (int i = 0; i < pTypes.length; i++) {
                        if (pTypes[i] == IntentFilter.class) params[i] = filter;
                        else if (BroadcastReceiver.class.isAssignableFrom(pTypes[i])) params[i] = receiver;
                        else if (pTypes[i] == String.class) params[i] = null;
                        else if (pTypes[i] == int.class) params[i] = 0;
                        else if (pTypes[i] == Context.class) params[i] = context;
                        else params[i] = null;
                    }
                    m.invoke(am, params);
                    log("AMS registerReceiver invoked successfully via Native AM!");
                    break;
                }
            }
        } catch (Throwable t) {
            log("AMS registerReceiver fallback: " + t.getMessage());
        }
    }

    private static void startCommandServer() {
        // 1. TCP Server on 127.0.0.1:8999
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ServerSocket ss = new ServerSocket(8999);
                    log("Command TCP Server listening on 127.0.0.1:8999");
                    while (true) {
                        Socket s = ss.accept();
                        BufferedReader r = new BufferedReader(new InputStreamReader(s.getInputStream()));
                        String line = r.readLine();
                        if (line != null) {
                            processCommand(line.trim());
                        }
                        s.close();
                    }
                } catch (Exception e) {
                    log("TCP Server error: " + e.getMessage());
                }
            }
        });
        t.setDaemon(true);
        t.start();

        // 2. Command File Watcher on /data/plugin/scoreboard_cmd
        Thread t2 = new Thread(new Runnable() {
            @Override
            public void run() {
                File f = new File("/data/plugin/scoreboard_cmd");
                long lastModified = (f.exists() && f.length() > 0) ? f.lastModified() : 0;
                while (true) {
                    try {
                        Thread.sleep(250);
                        if (f.exists() && f.length() > 0) {
                            long lm = f.lastModified();
                            if (lm != lastModified) {
                                lastModified = lm;
                                BufferedReader br = new BufferedReader(new FileReader(f));
                                String cmd = br.readLine();
                                br.close();
                                if (cmd != null && cmd.length() > 0) {
                                    processCommand(cmd.trim());
                                }
                            }
                        }
                    } catch (Exception ignored) {}
                }
            }
        });
        t2.setDaemon(true);
        t2.start();
    }

    public static void processCommand(String cmd) {
        log("Processing command: " + cmd);
        if ("hide".equalsIgnoreCase(cmd)) {
            setVisible(false);
        } else if ("show".equalsIgnoreCase(cmd)) {
            setVisible(true);
        } else if ("toggle".equalsIgnoreCase(cmd)) {
            setVisible(!mIsVisible);
        } else if ("reload_cfg".equalsIgnoreCase(cmd)) {
            mConfig = ScoreConfig.load();
            filterDisplayMatches();
            EspnFetcher.trigger();
            if (mTickerView != null) {
                mTickerView.post(new Runnable() {
                    @Override
                    public void run() {
                        mTickerView.invalidate();
                    }
                });
            }
            log("Configuration reloaded successfully");
        } else {
            setVisible(!mIsVisible);
        }
    }

    private static void populateMasterMatches() {
        synchronized (mAllMatches) {
            mAllMatches.clear();

            // 1. دوري أبطال أوروبا (UCL)
            mAllMatches.add(new MatchItem("ucl", "دوري أبطال أوروبا", "ريال مدريد", 2, 1, "مانشستر سيتي",
                    "الشوط 2 (78')", true,
                    "مبابي 23' ، فينيسيوس 67' | هالاند 41'",
                    "كارفخال 55'",
                    "رودري 72' (طرد)"));

            mAllMatches.add(new MatchItem("ucl", "دوري أبطال أوروبا", "بايرن ميونخ", 1, 0, "باريس سان جيرمان",
                    "الشوط 1 (38')", true,
                    "هاري كين 29'",
                    "كيميتش 34'", ""));

            // 2. الدوري الإنجليزي الممتاز (EPL)
            mAllMatches.add(new MatchItem("epl", "الدوري الإنجليزي", "أرسنال", 2, 2, "ليفربول",
                    "الشوط 2 (85')", true,
                    "ساكا 18' ، هافيرتز 59' | صلاح 44' ، دياز 75'",
                    "ساليبا 60' ، فان دايك 80'", ""));

            mAllMatches.add(new MatchItem("epl", "الدوري الإنجليزي", "مانشستر يونايتد", 0, 0, "تشيلسي",
                    "اليوم 21:00", false,
                    "", "", ""));

            // 3. الدوري الإسباني (La Liga)
            mAllMatches.add(new MatchItem("laliga", "الدوري الإسباني", "برشلونة", 3, 0, "إشبيلية",
                    "نهاية (FT)", false,
                    "ليفاندوفسكي 24'، 39' ، بيدري 82'",
                    "بادي 32'", ""));

            mAllMatches.add(new MatchItem("laliga", "الدوري الإسباني", "أتلتيكو مدريد", 1, 1, "فالنسيا",
                    "استراحة (HT)", true,
                    "غريزمان 15' | دورو 38'",
                    "كوكي 44'", ""));

            // 4. الدوري الإيطالي (Serie A)
            mAllMatches.add(new MatchItem("seriea", "الدوري الإيطالي", "إنتر ميلان", 2, 0, "يوفنتوس",
                    "نهاية (FT)", false,
                    "تورام 31' ، لاوتارو 64'",
                    "باريلا 28'",
                    "بريمر 59' (طرد)"));

            mAllMatches.add(new MatchItem("seriea", "الدوري الإيطالي", "ميلان", 0, 0, "روما",
                    "اليوم 21:45", false,
                    "", "", ""));

            // 5. البطولات العربية والمحلية (Arab)
            mAllMatches.add(new MatchItem("arab", "دوري روشن السعودي", "الهلال", 3, 2, "النصر",
                    "الشوط 2 (89')", true,
                    "ميتروفيتش 12'، 70' ، مالكوم 52' | رونالدو 34' ، ماني 61'",
                    "كوليبالي 45' ، بروزوفيتش 66'",
                    "لاجامي 81' (طرد)"));

            mAllMatches.add(new MatchItem("arab", "دوري أبطال أفريقيا", "الأهلي المصري", 1, 0, "الترجي التونسي",
                    "الشوط 2 (64')", true,
                    "وسام أبو علي 51'",
                    "مرياح 40'", ""));
        }
    }

    private static void filterDisplayMatches() {
        synchronized (mDisplayMatches) {
            mDisplayMatches.clear();
            synchronized (mAllMatches) {
                for (MatchItem m : mAllMatches) {
                    if ("ucl".equals(m.leagueKey) && !mConfig.showUcl) continue;
                    if ("epl".equals(m.leagueKey) && !mConfig.showEpl) continue;
                    if ("laliga".equals(m.leagueKey) && !mConfig.showLaliga) continue;
                    if ("seriea".equals(m.leagueKey) && !mConfig.showSeriea) continue;
                    if ("arab".equals(m.leagueKey) && !mConfig.showArab) continue;

                    if (mConfig.liveOnly && !m.isLive) continue;

                    mDisplayMatches.add(m);
                }
            }
        }
        if (mTickerView != null) {
            mTickerView.mNeedMeasure = true;
            mTickerView.mScrollX = 0f;
        }
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
        static {
            // Premier League
            TEAM_AR.put("Arsenal", "أرسنال");
            TEAM_AR.put("Aston Villa", "أستون فيلا");
            TEAM_AR.put("Brentford", "برينتفورد");
            TEAM_AR.put("Brighton & Hove Albion", "برايتون");
            TEAM_AR.put("Brighton", "برايتون");
            TEAM_AR.put("Chelsea", "تشيلسي");
            TEAM_AR.put("Crystal Palace", "كريستال بالاس");
            TEAM_AR.put("Everton", "إيفرتون");
            TEAM_AR.put("Fulham", "فولهام");
            TEAM_AR.put("Liverpool", "ليفربول");
            TEAM_AR.put("Manchester City", "مانشستر سيتي");
            TEAM_AR.put("Manchester United", "مانشستر يونايتد");
            TEAM_AR.put("Newcastle United", "نيوكاسل");
            TEAM_AR.put("Nottingham Forest", "نوتينغهام فورست");
            TEAM_AR.put("Tottenham Hotspur", "توتنهام");
            TEAM_AR.put("West Ham United", "وست هام");
            TEAM_AR.put("Wolverhampton Wanderers", "وولفرهامبتون");
            TEAM_AR.put("Leicester City", "ليستر سيتي");
            TEAM_AR.put("Southampton", "ساوثهامبتون");
            TEAM_AR.put("Ipswich Town", "إبسويتش تاون");
            TEAM_AR.put("Leeds United", "ليدز يونايتد");

            // La Liga
            TEAM_AR.put("Real Madrid", "ريال مدريد");
            TEAM_AR.put("Barcelona", "برشلونة");
            TEAM_AR.put("Atlético Madrid", "أتلتيكو مدريد");
            TEAM_AR.put("Atletico Madrid", "أتلتيكو مدريد");
            TEAM_AR.put("Sevilla", "إشبيلية");
            TEAM_AR.put("Real Sociedad", "ريال سوسيداد");
            TEAM_AR.put("Athletic Club", "أتلتيك بلباو");
            TEAM_AR.put("Villarreal", "فياريال");
            TEAM_AR.put("Real Betis", "ريال بيتيس");
            TEAM_AR.put("Valencia", "فالنسيا");
            TEAM_AR.put("Girona", "جيرونا");
            TEAM_AR.put("Mallorca", "مايوركا");
            TEAM_AR.put("Osasuna", "أوساسونا");
            TEAM_AR.put("Celta Vigo", "سيلتا فيغو");
            TEAM_AR.put("Getafe", "خيتافي");
            TEAM_AR.put("Espanyol", "إسبانيول");
            TEAM_AR.put("Málaga", "مالقا");
            TEAM_AR.put("Malaga", "مالقا");

            // Serie A
            TEAM_AR.put("Internazionale", "إنتر ميلان");
            TEAM_AR.put("Inter Milan", "إنتر ميلان");
            TEAM_AR.put("Juventus", "يوفنتوس");
            TEAM_AR.put("AC Milan", "ميلان");
            TEAM_AR.put("Milan", "ميلان");
            TEAM_AR.put("Napoli", "نابولي");
            TEAM_AR.put("Roma", "روما");
            TEAM_AR.put("Lazio", "لاتسيو");
            TEAM_AR.put("Atalanta", "أتالانتا");
            TEAM_AR.put("Fiorentina", "فيورنتينا");
            TEAM_AR.put("Torino", "تورينو");
            TEAM_AR.put("Bologna", "بولونيا");
            TEAM_AR.put("Genoa", "جنوى");
            TEAM_AR.put("Parma", "بارما");

            // UCL / European
            TEAM_AR.put("Bayern Munich", "بايرن ميونخ");
            TEAM_AR.put("Borussia Dortmund", "بوروسيا دورتموند");
            TEAM_AR.put("Bayer Leverkusen", "باير ليفركوزن");
            TEAM_AR.put("RB Leipzig", "لايبزيغ");
            TEAM_AR.put("Paris Saint-Germain", "باريس سان جيرمان");
            TEAM_AR.put("Monaco", "موناكو");
            TEAM_AR.put("Lille", "ليل");
            TEAM_AR.put("Lens", "لانس");
            TEAM_AR.put("Benfica", "بنفيكا");
            TEAM_AR.put("Sporting CP", "سبورتينغ لشبونة");
            TEAM_AR.put("Porto", "بورتو");
            TEAM_AR.put("Ajax", "أياكس");
            TEAM_AR.put("PSV Eindhoven", "بي إس في آيندهوفن");
            TEAM_AR.put("Feyenoord", "فينورد");
            TEAM_AR.put("Celtic", "سيلتيك");
            TEAM_AR.put("Galatasaray", "غلطة سراي");
            TEAM_AR.put("Fenerbahce", "فنربخشة");
            TEAM_AR.put("Club Brugge", "كلوب بروج");

            // Arab & Local
            TEAM_AR.put("al hilal", "الهلال");
            TEAM_AR.put("al nassr", "النصر");
            TEAM_AR.put("al ittihad", "الاتحاد");
            TEAM_AR.put("al ahli", "الأهلي السعودي");
            TEAM_AR.put("al ahly", "الأهلي المصري");
            TEAM_AR.put("zamalek", "الزمالك");
            TEAM_AR.put("esperance de tunis", "الترجي التونسي");
            TEAM_AR.put("esperance", "الترجي التونسي");
            TEAM_AR.put("wydad casablanca", "الوداد البيضاوي");
            TEAM_AR.put("raja casablanca", "الرجاء البيضاوي");
            TEAM_AR.put("al shabab", "الشباب");
            TEAM_AR.put("al ettifaq", "الاتفاق");
            TEAM_AR.put("al fateh", "الفتح");
            TEAM_AR.put("al taawoun", "التعاون");
            TEAM_AR.put("al qadsiah", "القادسية");
            TEAM_AR.put("al wehda", "الوحدة");
            TEAM_AR.put("al fayha", "الفيحاء");
            TEAM_AR.put("damac", "ضمك");
            TEAM_AR.put("al raed", "الرائد");
            TEAM_AR.put("al kholood", "الخلود");
            TEAM_AR.put("al orobah", "العروبة");
            TEAM_AR.put("al riyadh", "الرياض");
            TEAM_AR.put("al akhdoud", "الأخدود");
            TEAM_AR.put("pyramids fc", "بيراميدز");
            TEAM_AR.put("al ain", "العين");
            TEAM_AR.put("al-quwa al-jawiya", "القوة الجوية");
            TEAM_AR.put("al quwa al jawiya", "القوة الجوية");
            TEAM_AR.put("neftchi fergana", "نيفتشي");
            TEAM_AR.put("al sadd", "السد");
            TEAM_AR.put("al rayyan", "الريان");
            TEAM_AR.put("al duhail", "الدحيل");
            TEAM_AR.put("al wasl", "الوصل");
            TEAM_AR.put("al sharjah", "الشارقة");
            TEAM_AR.put("shabab al ahli", "شباب الأهلي");
        }

        public static String translate(String name) {
            if (name == null || name.length() == 0) return "";
            String clean = name.trim();
            if (TEAM_AR.containsKey(clean)) return TEAM_AR.get(clean);
            String lower = clean.toLowerCase();
            if (TEAM_AR.containsKey(lower)) return TEAM_AR.get(lower);
            for (Map.Entry<String, String> e : TEAM_AR.entrySet()) {
                if (e.getKey().equalsIgnoreCase(clean)) return e.getValue();
            }
            String stripped = lower.replaceAll("(?i)\\s+(fc|cf|sc|ac)$", "").trim();
            for (Map.Entry<String, String> e : TEAM_AR.entrySet()) {
                if (e.getKey().equalsIgnoreCase(stripped)) return e.getValue();
            }
            return clean;
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
            log("EspnFetcher: Started background live data engine");

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

            // 1. UCL
            if (mConfig.showUcl) {
                fetchLeague("ucl", "uefa.champions", "دوري أبطال أوروبا", fresh);
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
            }
            // 5. Arab & Local
            if (mConfig.showArab) {
                fetchLeague("arab", "ksa.1", "دوري روشن السعودي", fresh);
                fetchLeague("arab", "caf.champions", "دوري أبطال أفريقيا", fresh);
                fetchLeague("arab", "afc.champions", "دوري أبطال آسيا", fresh);
            }

            if (!fresh.isEmpty()) {
                synchronized (mAllMatches) {
                    mAllMatches.clear();
                    mAllMatches.addAll(fresh);
                }
                filterDisplayMatches();
                if (mTickerView != null) {
                    mTickerView.postInvalidate();
                }
                log("EspnFetcher: Successfully updated " + fresh.size() + " matches live from ESPN!");
            }
        }

        private void fetchLeague(String lKey, String espnCode, String defaultName, List<MatchItem> outList) {
            HttpURLConnection conn = null;
            try {
                URL u = new URL("https://site.api.espn.com/apis/site/v2/sports/soccer/" + espnCode + "/scoreboard");
                conn = (HttpURLConnection) u.openConnection();
                conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android 7.0)");
                conn.setConnectTimeout(6000);
                conn.setReadTimeout(6000);

                int code = conn.getResponseCode();
                if (code != 200) {
                    log("EspnFetcher: " + espnCode + " HTTP " + code);
                    return;
                }

                BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = br.readLine()) != null) {
                    sb.append(line);
                }
                br.close();

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
                    if (status != null) {
                        clock = status.optString("displayClock", "");
                        JSONObject typeObj = status.optJSONObject("type");
                        if (typeObj != null) {
                            sType = typeObj.optString("name", "");
                            shortDetail = typeObj.optString("shortDetail", "مجدولة");
                        }
                    }

                    boolean isLive = false;
                    String statusText = "مجدولة";
                    if (sType.contains("IN_PROGRESS") || sType.contains("FIRST_HALF")) {
                        statusText = "الشوط 1 (" + (clock.isEmpty() ? "1'" : clock) + ")";
                        isLive = true;
                    } else if (sType.contains("HALFTIME")) {
                        statusText = "استراحة (HT)";
                        isLive = true;
                    } else if (sType.contains("SECOND_HALF")) {
                        statusText = "الشوط 2 (" + (clock.isEmpty() ? "45'" : clock) + ")";
                        isLive = true;
                    } else if (sType.contains("EXTRA_TIME") || sType.contains("OVERTIME")) {
                        statusText = "إضافي (" + clock + ")";
                        isLive = true;
                    } else if (sType.contains("SHOOTOUT") || sType.contains("PENALTIES")) {
                        statusText = "ركلات ترجيح";
                        isLive = true;
                    } else if (sType.contains("FINAL") || sType.contains("FULL_TIME")) {
                        statusText = "نهاية (FT)";
                        isLive = false;
                    } else if (sType.contains("POSTPONED")) {
                        statusText = "مؤجلة";
                        isLive = false;
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

                    String t1Raw = home.optJSONObject("team") != null ? home.getJSONObject("team").optString("displayName", "فريق 1") : "فريق 1";
                    String t2Raw = away.optJSONObject("team") != null ? away.getJSONObject("team").optString("displayName", "فريق 2") : "فريق 2";

                    String t1 = translate(t1Raw);
                    String t2 = translate(t2Raw);

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

                    outList.add(new MatchItem(lKey, compName, t1, s1, s2, t2, statusText, isLive, scorers, yellows, reds));
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

    public static void handleIntent(Intent intent) {
        String action = intent != null ? intent.getAction() : "";
        log("Received intent action: " + action);

        if (intent.hasExtra("hide")) {
            boolean hide = intent.getBooleanExtra("hide", false);
            setVisible(!hide);
            return;
        }

        if (intent.hasExtra("show")) {
            boolean show = intent.getBooleanExtra("show", true);
            setVisible(show);
            return;
        }

        if (intent.hasExtra("reload_cfg")) {
            mConfig = ScoreConfig.load();
            filterDisplayMatches();
            EspnFetcher.trigger();
            if (mTickerView != null) mTickerView.invalidate();
            return;
        }

        setVisible(!mIsVisible);
    }

    private static void setVisible(boolean visible) {
        if (mHandler != null) {
            mHandler.post(new VisTask(visible));
        }
    }

    public static void log(String msg) {
        System.out.println("[ScoreBoardHud] " + msg);
    }
}
