#include "nomi_eyes.h"

#include <time.h>

#define SCREEN_SIZE 240
#define SCREEN_CENTER 120
#define EYE_CENTER_Y 103
#define EYE_OFFSET_X 42
#define DEFAULT_CAROUSEL_PERIOD_MS 2200
#define MOTION_PERIOD_MS 50
#define CLOCK_CHECK_PERIOD_MS 250
#define CLOCK_PARTICLE_COUNT 96
#define CLOCK_MORPH_MS 720
#define CLOCK_HOLD_MS 1700
#define CLOCK_RETURN_MS 480
#define CLOCK_TOTAL_MS (CLOCK_MORPH_MS + CLOCK_HOLD_MS + CLOCK_RETURN_MS)

typedef enum {
    EYE_SHAPE_CAPSULE = 0,
    EYE_SHAPE_ARC_UP,
    EYE_SHAPE_ARC_DOWN,
    EYE_SHAPE_RING,
    EYE_SHAPE_X,
    EYE_SHAPE_HEART,
    EYE_SHAPE_STAR,
    EYE_SHAPE_SQUARE,
    EYE_SHAPE_DIAMOND,
    EYE_SHAPE_SPIRAL
} eye_shape_t;

typedef enum {
    MOTION_STILL = 0,
    MOTION_CALM,
    MOTION_SLOW,
    MOTION_WAKE,
    MOTION_PULSE,
    MOTION_THINK,
    MOTION_SCAN,
    MOTION_NOD,
    MOTION_SHAKE,
    MOTION_HAPPY,
    MOTION_BOUNCE,
    MOTION_CURIOUS,
    MOTION_ASYMMETRIC,
    MOTION_FAST_PULSE,
    MOTION_SHY,
    MOTION_DROOP,
    MOTION_SLEEPY,
    MOTION_SLEEP,
    MOTION_JITTER,
    MOTION_SWAY,
    MOTION_TREMBLE,
    MOTION_PANIC
} motion_kind_t;

typedef enum {
    EFFECT_NONE = 0,
    EFFECT_SOUND,
    EFFECT_DOTS,
    EFFECT_CHECK,
    EFFECT_HASH,
    EFFECT_SPARKLES,
    EFFECT_QUESTION,
    EFFECT_SWEAT,
    EFFECT_EXCLAMATION,
    EFFECT_HEARTS,
    EFFECT_BREATH,
    EFFECT_Z,
    EFFECT_ZZZ,
    EFFECT_TEARS,
    EFFECT_ONE_TEAR,
    EFFECT_SHINE,
    EFFECT_CONFETTI,
    EFFECT_STEAM,
    EFFECT_STRESS,
    EFFECT_BURST,
    EFFECT_SCAN,
    EFFECT_SNOW,
    EFFECT_STARS,
    EFFECT_RAGE
} accessory_effect_t;

typedef struct {
    eye_shape_t shape;
    int16_t width;
    int16_t height;
    int16_t x;
    int16_t y;
    int16_t tilt;
} eye_visual_t;

typedef struct {
    const char * name;
    eye_visual_t left;
    eye_visual_t right;
    int16_t gaze_x;
    int16_t gaze_y;
    bool auto_blink;
    motion_kind_t motion;
    accessory_effect_t effect;
} expression_def_t;

typedef struct {
    int16_t pair_x;
    int16_t pair_y;
    int16_t left_x;
    int16_t left_y;
    int16_t right_x;
    int16_t right_y;
    int16_t size;
} motion_offset_t;

typedef struct {
    int16_t start_x;
    int16_t start_y;
    int16_t target_x;
    int16_t target_y;
    uint8_t size;
    uint8_t delay;
} clock_particle_t;

typedef struct {
    lv_obj_t * canvas;
    lv_timer_t * carousel_timer;
    lv_timer_t * clock_timer;
    nomi_expression_t current;
    nomi_expression_t pending;
    uint32_t motion_started;
    uint32_t clock_started;
    int32_t blink;
    int32_t touch_gaze_x;
    int32_t touch_gaze_y;
    int32_t last_chime_stamp;
    int32_t clock_hour;
    int32_t clock_minute;
    bool pressed;
    bool transitioning;
    bool clock_active;
    clock_particle_t clock_particles[CLOCK_PARTICLE_COUNT];
} nomi_eye_state_t;

#define EYE(shape_, width_, height_, x_, y_, tilt_) \
    { (shape_), (width_), (height_), (x_), (y_), (tilt_) }
#define EXPR(name_, left_, right_, gx_, gy_, blink_, motion_, effect_) \
    { (name_), left_, right_, (gx_), (gy_), (blink_), (motion_), (effect_) }

static const expression_def_t expressions[NOMI_EXPRESSION_COUNT] = {
    [NOMI_EXPRESSION_IDLE] = EXPR("Idle",
        EYE(EYE_SHAPE_CAPSULE, 50, 34, 0, 0, 0), EYE(EYE_SHAPE_CAPSULE, 50, 34, 0, 0, 0),
        0, 0, true, MOTION_CALM, EFFECT_NONE),
    [NOMI_EXPRESSION_RELAXED] = EXPR("Relaxed",
        EYE(EYE_SHAPE_CAPSULE, 52, 18, 0, 2, 0), EYE(EYE_SHAPE_CAPSULE, 52, 18, 0, 2, 0),
        0, 0, true, MOTION_SLOW, EFFECT_NONE),
    [NOMI_EXPRESSION_WAKE] = EXPR("Wake",
        EYE(EYE_SHAPE_CAPSULE, 48, 40, 0, -2, 0), EYE(EYE_SHAPE_CAPSULE, 48, 40, 0, -2, 0),
        0, 0, true, MOTION_WAKE, EFFECT_NONE),
    [NOMI_EXPRESSION_LISTENING] = EXPR("Listening",
        EYE(EYE_SHAPE_CAPSULE, 42, 38, 2, 0, 0), EYE(EYE_SHAPE_CAPSULE, 42, 38, -2, 0, 0),
        0, 0, true, MOTION_PULSE, EFFECT_SOUND),
    [NOMI_EXPRESSION_THINKING] = EXPR("Thinking",
        EYE(EYE_SHAPE_CAPSULE, 48, 20, 0, 0, -2), EYE(EYE_SHAPE_CAPSULE, 48, 20, 0, 0, 2),
        -6, -4, true, MOTION_THINK, EFFECT_DOTS),
    [NOMI_EXPRESSION_RESPONDING] = EXPR("Responding",
        EYE(EYE_SHAPE_CAPSULE, 46, 25, 0, 0, 0), EYE(EYE_SHAPE_CAPSULE, 46, 25, 0, 0, 0),
        0, 0, true, MOTION_SCAN, EFFECT_SOUND),
    [NOMI_EXPRESSION_CONFIRM] = EXPR("Confirm",
        EYE(EYE_SHAPE_ARC_UP, 48, 9, 0, 0, 0), EYE(EYE_SHAPE_ARC_UP, 48, 9, 0, 0, 0),
        0, 0, false, MOTION_NOD, EFFECT_CHECK),
    [NOMI_EXPRESSION_ANGRY] = EXPR("Angry",
        EYE(EYE_SHAPE_CAPSULE, 50, 10, 0, 0, 9), EYE(EYE_SHAPE_CAPSULE, 50, 10, 0, 0, -9),
        0, 0, false, MOTION_SHAKE, EFFECT_HASH),
    [NOMI_EXPRESSION_HAPPY] = EXPR("Happy",
        EYE(EYE_SHAPE_ARC_UP, 52, 10, 0, 0, 0), EYE(EYE_SHAPE_ARC_UP, 52, 10, 0, 0, 0),
        0, 0, false, MOTION_HAPPY, EFFECT_NONE),
    [NOMI_EXPRESSION_EXCITED] = EXPR("Excited",
        EYE(EYE_SHAPE_ARC_UP, 56, 12, 0, -2, 0), EYE(EYE_SHAPE_ARC_UP, 56, 12, 0, -2, 0),
        0, 0, false, MOTION_BOUNCE, EFFECT_SPARKLES),
    [NOMI_EXPRESSION_CURIOUS] = EXPR("Curious",
        EYE(EYE_SHAPE_CAPSULE, 46, 38, 0, -1, 0), EYE(EYE_SHAPE_CAPSULE, 42, 24, 0, 2, -3),
        6, -1, true, MOTION_CURIOUS, EFFECT_QUESTION),
    [NOMI_EXPRESSION_SPEECHLESS] = EXPR("Speechless",
        EYE(EYE_SHAPE_CAPSULE, 48, 16, 0, 0, 7), EYE(EYE_SHAPE_CAPSULE, 40, 30, 0, 1, -3),
        -2, 0, true, MOTION_ASYMMETRIC, EFFECT_SWEAT),
    [NOMI_EXPRESSION_SURPRISED] = EXPR("Surprised",
        EYE(EYE_SHAPE_CAPSULE, 38, 42, 0, -1, 0), EYE(EYE_SHAPE_CAPSULE, 38, 42, 0, -1, 0),
        0, 0, true, MOTION_FAST_PULSE, EFFECT_EXCLAMATION),
    [NOMI_EXPRESSION_SHY] = EXPR("Shy",
        EYE(EYE_SHAPE_ARC_UP, 42, 8, 4, 3, 0), EYE(EYE_SHAPE_ARC_UP, 42, 8, -4, 3, 0),
        4, 3, false, MOTION_SHY, EFFECT_HEARTS),
    [NOMI_EXPRESSION_SIGH] = EXPR("Sigh",
        EYE(EYE_SHAPE_ARC_DOWN, 48, 8, 0, 3, 0), EYE(EYE_SHAPE_ARC_DOWN, 48, 8, 0, 3, 0),
        0, 3, false, MOTION_DROOP, EFFECT_BREATH),
    [NOMI_EXPRESSION_SLEEPY] = EXPR("Sleepy",
        EYE(EYE_SHAPE_CAPSULE, 50, 10, 0, 4, 1), EYE(EYE_SHAPE_CAPSULE, 48, 6, 0, 5, -1),
        0, 2, true, MOTION_SLEEPY, EFFECT_Z),
    [NOMI_EXPRESSION_SLEEP] = EXPR("Sleep",
        EYE(EYE_SHAPE_CAPSULE, 46, 4, 0, 7, 2), EYE(EYE_SHAPE_CAPSULE, 46, 4, 0, 7, -2),
        0, 2, false, MOTION_SLEEP, EFFECT_ZZZ),
    [NOMI_EXPRESSION_ALERT] = EXPR("Alert",
        EYE(EYE_SHAPE_CAPSULE, 52, 10, 0, -2, 11), EYE(EYE_SHAPE_CAPSULE, 52, 10, 0, -2, -11),
        0, -1, false, MOTION_JITTER, EFFECT_EXCLAMATION),
    [NOMI_EXPRESSION_GENTLE_SMILE] = EXPR("Gentle Smile",
        EYE(EYE_SHAPE_ARC_UP, 46, 8, 0, 2, 0), EYE(EYE_SHAPE_ARC_UP, 46, 8, 0, 2, 0),
        0, 0, false, MOTION_CALM, EFFECT_NONE),
    [NOMI_EXPRESSION_BIG_GRIN] = EXPR("Big Grin",
        EYE(EYE_SHAPE_ARC_UP, 60, 13, 0, -2, 0), EYE(EYE_SHAPE_ARC_UP, 60, 13, 0, -2, 0),
        0, 0, false, MOTION_BOUNCE, EFFECT_NONE),
    [NOMI_EXPRESSION_LAUGH] = EXPR("Laugh",
        EYE(EYE_SHAPE_ARC_UP, 54, 9, 0, 0, 0), EYE(EYE_SHAPE_ARC_UP, 54, 9, 0, 0, 0),
        0, 0, false, MOTION_BOUNCE, EFFECT_SPARKLES),
    [NOMI_EXPRESSION_TEARS_OF_JOY] = EXPR("Tears of Joy",
        EYE(EYE_SHAPE_ARC_UP, 52, 10, 0, 0, 0), EYE(EYE_SHAPE_ARC_UP, 52, 10, 0, 0, 0),
        0, 0, false, MOTION_HAPPY, EFFECT_TEARS),
    [NOMI_EXPRESSION_WINK] = EXPR("Wink",
        EYE(EYE_SHAPE_CAPSULE, 42, 30, 0, 0, 0), EYE(EYE_SHAPE_CAPSULE, 46, 5, 0, 3, -2),
        0, 0, true, MOTION_CALM, EFFECT_SHINE),
    [NOMI_EXPRESSION_PLAYFUL] = EXPR("Playful",
        EYE(EYE_SHAPE_RING, 42, 42, 0, -1, 0), EYE(EYE_SHAPE_CAPSULE, 46, 6, 0, 3, 2),
        4, 0, true, MOTION_SWAY, EFFECT_SPARKLES),
    [NOMI_EXPRESSION_COOL] = EXPR("Cool",
        EYE(EYE_SHAPE_CAPSULE, 56, 8, 0, 1, 0), EYE(EYE_SHAPE_CAPSULE, 56, 8, 0, 1, 0),
        3, 0, false, MOTION_SWAY, EFFECT_SHINE),
    [NOMI_EXPRESSION_IN_LOVE] = EXPR("In Love",
        EYE(EYE_SHAPE_HEART, 36, 36, 0, 0, 0), EYE(EYE_SHAPE_HEART, 36, 36, 0, 0, 0),
        0, 0, false, MOTION_PULSE, EFFECT_HEARTS),
    [NOMI_EXPRESSION_STARSTRUCK] = EXPR("Starstruck",
        EYE(EYE_SHAPE_STAR, 38, 38, 0, 0, 0), EYE(EYE_SHAPE_STAR, 38, 38, 0, 0, 0),
        0, 0, false, MOTION_FAST_PULSE, EFFECT_SPARKLES),
    [NOMI_EXPRESSION_KISSY] = EXPR("Kissy",
        EYE(EYE_SHAPE_CAPSULE, 40, 28, 0, 0, 0), EYE(EYE_SHAPE_CAPSULE, 45, 5, 0, 3, -2),
        2, 0, true, MOTION_CALM, EFFECT_HEARTS),
    [NOMI_EXPRESSION_WARM_HUG] = EXPR("Warm Hug",
        EYE(EYE_SHAPE_ARC_UP, 48, 9, 3, 2, 0), EYE(EYE_SHAPE_ARC_UP, 48, 9, -3, 2, 0),
        0, 0, false, MOTION_SHY, EFFECT_HEARTS),
    [NOMI_EXPRESSION_BLISSFUL] = EXPR("Blissful",
        EYE(EYE_SHAPE_ARC_UP, 50, 8, 0, 2, 0), EYE(EYE_SHAPE_ARC_UP, 50, 8, 0, 2, 0),
        0, 0, false, MOTION_SLOW, EFFECT_SPARKLES),
    [NOMI_EXPRESSION_PROUD] = EXPR("Proud",
        EYE(EYE_SHAPE_CAPSULE, 48, 12, 0, 0, -5), EYE(EYE_SHAPE_CAPSULE, 48, 12, 0, 0, 5),
        0, -2, false, MOTION_SLOW, EFFECT_STEAM),
    [NOMI_EXPRESSION_MISCHIEVOUS] = EXPR("Mischievous",
        EYE(EYE_SHAPE_CAPSULE, 52, 10, 0, 0, -8), EYE(EYE_SHAPE_CAPSULE, 46, 15, 0, 1, -3),
        6, 0, true, MOTION_SWAY, EFFECT_SHINE),
    [NOMI_EXPRESSION_SILLY] = EXPR("Silly",
        EYE(EYE_SHAPE_RING, 40, 40, 0, 0, 0), EYE(EYE_SHAPE_X, 34, 34, 0, 1, 0),
        0, 0, false, MOTION_ASYMMETRIC, EFFECT_NONE),
    [NOMI_EXPRESSION_PARTY] = EXPR("Party",
        EYE(EYE_SHAPE_ARC_UP, 54, 11, 0, -1, 0), EYE(EYE_SHAPE_ARC_UP, 54, 11, 0, -1, 0),
        0, 0, false, MOTION_BOUNCE, EFFECT_CONFETTI),
    [NOMI_EXPRESSION_MAGICAL] = EXPR("Magical",
        EYE(EYE_SHAPE_STAR, 38, 38, 0, 0, 0), EYE(EYE_SHAPE_RING, 38, 38, 0, 0, 0),
        0, 0, false, MOTION_SWAY, EFFECT_STARS),
    [NOMI_EXPRESSION_THANKFUL] = EXPR("Thankful",
        EYE(EYE_SHAPE_ARC_UP, 44, 8, 1, 3, 0), EYE(EYE_SHAPE_ARC_UP, 44, 8, -1, 3, 0),
        0, 0, false, MOTION_CALM, EFFECT_HEARTS),
    [NOMI_EXPRESSION_SKEPTICAL] = EXPR("Skeptical",
        EYE(EYE_SHAPE_CAPSULE, 52, 10, 0, 1, 6), EYE(EYE_SHAPE_CAPSULE, 38, 28, 0, 0, 0),
        -2, 0, true, MOTION_CURIOUS, EFFECT_QUESTION),
    [NOMI_EXPRESSION_UNAMUSED] = EXPR("Unamused",
        EYE(EYE_SHAPE_CAPSULE, 52, 12, 0, 2, 0), EYE(EYE_SHAPE_CAPSULE, 52, 12, 0, 2, 0),
        -6, 2, true, MOTION_SLOW, EFFECT_DOTS),
    [NOMI_EXPRESSION_EYE_ROLL] = EXPR("Eye Roll",
        EYE(EYE_SHAPE_RING, 34, 34, 0, 0, 0), EYE(EYE_SHAPE_RING, 34, 34, 0, 0, 0),
        0, -8, true, MOTION_SWAY, EFFECT_NONE),
    [NOMI_EXPRESSION_DOUBTFUL] = EXPR("Doubtful",
        EYE(EYE_SHAPE_ARC_DOWN, 44, 7, 0, 1, 0), EYE(EYE_SHAPE_CAPSULE, 42, 18, 0, 0, -3),
        0, 0, true, MOTION_ASYMMETRIC, EFFECT_QUESTION),
    [NOMI_EXPRESSION_AWKWARD] = EXPR("Awkward",
        EYE(EYE_SHAPE_CAPSULE, 32, 12, 0, 2, 0), EYE(EYE_SHAPE_CAPSULE, 32, 12, 0, 2, 0),
        5, 1, true, MOTION_JITTER, EFFECT_SWEAT),
    [NOMI_EXPRESSION_EMBARRASSED] = EXPR("Embarrassed",
        EYE(EYE_SHAPE_ARC_UP, 40, 7, 2, 3, 0), EYE(EYE_SHAPE_ARC_UP, 40, 7, -2, 5, 0),
        3, 2, false, MOTION_SHY, EFFECT_SWEAT),
    [NOMI_EXPRESSION_BORED] = EXPR("Bored",
        EYE(EYE_SHAPE_CAPSULE, 54, 8, 0, 4, 0), EYE(EYE_SHAPE_CAPSULE, 54, 8, 0, 4, 0),
        -5, 2, true, MOTION_SLOW, EFFECT_DOTS),
    [NOMI_EXPRESSION_FOCUSED] = EXPR("Focused",
        EYE(EYE_SHAPE_RING, 36, 36, 0, 0, 0), EYE(EYE_SHAPE_RING, 36, 36, 0, 0, 0),
        0, 0, true, MOTION_PULSE, EFFECT_SCAN),
    [NOMI_EXPRESSION_DETERMINED] = EXPR("Determined",
        EYE(EYE_SHAPE_CAPSULE, 54, 11, 0, 0, 8), EYE(EYE_SHAPE_CAPSULE, 54, 11, 0, 0, -8),
        0, 0, false, MOTION_JITTER, EFFECT_STEAM),
    [NOMI_EXPRESSION_SUSPICIOUS] = EXPR("Suspicious",
        EYE(EYE_SHAPE_CAPSULE, 52, 9, 0, 1, 1), EYE(EYE_SHAPE_CAPSULE, 46, 14, 0, 0, -2),
        -8, 0, true, MOTION_SCAN, EFFECT_NONE),
    [NOMI_EXPRESSION_SEARCHING] = EXPR("Searching",
        EYE(EYE_SHAPE_RING, 46, 46, 0, 0, 0), EYE(EYE_SHAPE_CAPSULE, 34, 24, 0, 1, 0),
        5, 0, true, MOTION_SCAN, EFFECT_QUESTION),
    [NOMI_EXPRESSION_ROBOT] = EXPR("Robot",
        EYE(EYE_SHAPE_SQUARE, 34, 28, 0, 0, 0), EYE(EYE_SHAPE_SQUARE, 34, 28, 0, 0, 0),
        0, 0, false, MOTION_SCAN, EFFECT_SCAN),
    [NOMI_EXPRESSION_WORRIED] = EXPR("Worried",
        EYE(EYE_SHAPE_ARC_DOWN, 48, 8, 0, 2, 0), EYE(EYE_SHAPE_ARC_DOWN, 48, 8, 0, 2, 0),
        0, 2, false, MOTION_DROOP, EFFECT_SWEAT),
    [NOMI_EXPRESSION_ANXIOUS] = EXPR("Anxious",
        EYE(EYE_SHAPE_CAPSULE, 40, 36, 0, 0, 0), EYE(EYE_SHAPE_CAPSULE, 36, 30, 0, 2, 0),
        0, 0, true, MOTION_TREMBLE, EFFECT_SWEAT),
    [NOMI_EXPRESSION_NERVOUS] = EXPR("Nervous",
        EYE(EYE_SHAPE_CAPSULE, 48, 10, 0, 2, 2), EYE(EYE_SHAPE_CAPSULE, 48, 10, 0, 2, -2),
        0, 1, true, MOTION_TREMBLE, EFFECT_STRESS),
    [NOMI_EXPRESSION_AFRAID] = EXPR("Afraid",
        EYE(EYE_SHAPE_RING, 42, 42, 0, 0, 0), EYE(EYE_SHAPE_RING, 42, 42, 0, 0, 0),
        0, 0, true, MOTION_TREMBLE, EFFECT_SWEAT),
    [NOMI_EXPRESSION_PANIC] = EXPR("Panic",
        EYE(EYE_SHAPE_RING, 50, 50, 0, 0, 0), EYE(EYE_SHAPE_RING, 50, 50, 0, 0, 0),
        0, 0, false, MOTION_PANIC, EFFECT_BURST),
    [NOMI_EXPRESSION_SAD] = EXPR("Sad",
        EYE(EYE_SHAPE_ARC_DOWN, 50, 8, 0, 3, 0), EYE(EYE_SHAPE_ARC_DOWN, 50, 8, 0, 3, 0),
        0, 3, false, MOTION_DROOP, EFFECT_ONE_TEAR),
    [NOMI_EXPRESSION_CRYING] = EXPR("Crying",
        EYE(EYE_SHAPE_ARC_DOWN, 52, 9, 0, 2, 0), EYE(EYE_SHAPE_ARC_DOWN, 52, 9, 0, 2, 0),
        0, 2, false, MOTION_TREMBLE, EFFECT_TEARS),
    [NOMI_EXPRESSION_PLEADING] = EXPR("Pleading",
        EYE(EYE_SHAPE_RING, 48, 48, 3, 0, 0), EYE(EYE_SHAPE_RING, 48, 48, -3, 0, 0),
        0, 2, true, MOTION_PULSE, EFFECT_ONE_TEAR),
    [NOMI_EXPRESSION_DISAPPOINTED] = EXPR("Disappointed",
        EYE(EYE_SHAPE_ARC_DOWN, 46, 7, 0, 5, 0), EYE(EYE_SHAPE_ARC_DOWN, 46, 7, 0, 5, 0),
        0, 3, false, MOTION_SLOW, EFFECT_DOTS),
    [NOMI_EXPRESSION_LONELY] = EXPR("Lonely",
        EYE(EYE_SHAPE_ARC_DOWN, 36, 6, 5, 5, 0), EYE(EYE_SHAPE_ARC_DOWN, 36, 6, -5, 5, 0),
        0, 3, false, MOTION_SLOW, EFFECT_NONE),
    [NOMI_EXPRESSION_FRUSTRATED] = EXPR("Frustrated",
        EYE(EYE_SHAPE_X, 36, 36, 0, 0, 0), EYE(EYE_SHAPE_X, 36, 36, 0, 0, 0),
        0, 0, false, MOTION_SHAKE, EFFECT_HASH),
    [NOMI_EXPRESSION_DIZZY] = EXPR("Dizzy",
        EYE(EYE_SHAPE_SPIRAL, 40, 40, 0, 0, 0), EYE(EYE_SHAPE_SPIRAL, 40, 40, 0, 0, 0),
        0, 0, false, MOTION_SWAY, EFFECT_STARS),
    [NOMI_EXPRESSION_KNOCKED_OUT] = EXPR("Knocked Out",
        EYE(EYE_SHAPE_X, 42, 42, 0, 1, 0), EYE(EYE_SHAPE_X, 42, 42, 0, 1, 0),
        0, 1, false, MOTION_STILL, EFFECT_NONE),
    [NOMI_EXPRESSION_SICK] = EXPR("Sick",
        EYE(EYE_SHAPE_CAPSULE, 48, 10, 0, 4, 2), EYE(EYE_SHAPE_CAPSULE, 44, 7, 0, 5, -2),
        -2, 2, true, MOTION_DROOP, EFFECT_SWEAT),
    [NOMI_EXPRESSION_FREEZING] = EXPR("Freezing",
        EYE(EYE_SHAPE_CAPSULE, 50, 8, 0, 2, 2), EYE(EYE_SHAPE_CAPSULE, 50, 8, 0, 2, -2),
        0, 1, true, MOTION_TREMBLE, EFFECT_SNOW),
    [NOMI_EXPRESSION_RAGE] = EXPR("Rage",
        EYE(EYE_SHAPE_CAPSULE, 58, 13, 0, -1, 12), EYE(EYE_SHAPE_CAPSULE, 58, 13, 0, -1, -12),
        0, -1, false, MOTION_JITTER, EFFECT_RAGE),
};

static nomi_eye_state_t state;

static int32_t clamp_i32(int32_t value, int32_t low, int32_t high)
{
    if(value < low) return low;
    if(value > high) return high;
    return value;
}

static int32_t wave(uint32_t elapsed, uint32_t period, int32_t amplitude, int32_t phase)
{
    int32_t angle = (int32_t)(((elapsed % period) * 360U) / period) + phase;
    return (amplitude * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT;
}

static int32_t lerp_i32(int32_t from, int32_t to, int32_t progress)
{
    return from + ((to - from) * progress) / 1000;
}

static int32_t ease_out_cubic_i32(int32_t progress)
{
    int32_t inv = 1000 - progress;
    return 1000 - (inv * inv * inv) / 1000000;
}

static bool get_local_clock(int32_t * hour, int32_t * minute,
                            int32_t * second, int32_t * yday)
{
    time_t now = time(NULL);
    struct tm * tm_now = localtime(&now);
    if(!tm_now) return false;

    *hour = tm_now->tm_hour;
    *minute = tm_now->tm_min;
    *second = tm_now->tm_sec;
    *yday = tm_now->tm_yday;
    return true;
}

static void invalidate_eyes(void)
{
    if(state.canvas) lv_obj_invalidate(state.canvas);
}

static void calculate_motion(motion_offset_t * motion)
{
    lv_memzero(motion, sizeof(*motion));
    uint32_t elapsed = lv_tick_elaps(state.motion_started);
    motion_kind_t kind = expressions[state.current].motion;

    switch(kind) {
        case MOTION_CALM:
            motion->pair_y = (int16_t)wave(elapsed, 1800, 1, 0);
            break;
        case MOTION_SLOW:
            motion->pair_y = (int16_t)wave(elapsed, 2400, 1, 0);
            break;
        case MOTION_WAKE:
            if(elapsed < 900U) {
                int32_t bounce = LV_ABS(wave(elapsed, 450, 4, 0));
                motion->pair_y = (int16_t)(-(bounce * (int32_t)(900U - elapsed) / 900));
            }
            break;
        case MOTION_PULSE:
            motion->size = (int16_t)wave(elapsed, 1100, 1, 0);
            break;
        case MOTION_THINK:
            motion->pair_x = (int16_t)wave(elapsed, 1600, 3, 0);
            motion->left_y = (int16_t)wave(elapsed, 2100, 1, 90);
            motion->right_y = motion->left_y;
            break;
        case MOTION_SCAN:
            motion->pair_x = (int16_t)wave(elapsed, 900, 6, 0);
            break;
        case MOTION_NOD:
            if(elapsed < 800U) motion->pair_y = (int16_t)LV_ABS(wave(elapsed, 800, 4, 0));
            break;
        case MOTION_SHAKE:
            if(elapsed < 1000U) motion->pair_x = (int16_t)wave(elapsed, 180, 4, 0);
            break;
        case MOTION_HAPPY:
            motion->pair_y = (int16_t)wave(elapsed, 1800, 1, 0);
            break;
        case MOTION_BOUNCE:
            motion->pair_y = (int16_t)(-LV_ABS(wave(elapsed, 560, 4, 0)));
            motion->size = (int16_t)LV_ABS(wave(elapsed, 560, 1, 90));
            break;
        case MOTION_CURIOUS:
            motion->pair_x = (int16_t)wave(elapsed, 1500, 2, 0);
            break;
        case MOTION_ASYMMETRIC:
            motion->left_y = (int16_t)wave(elapsed, 1000, 2, 0);
            motion->right_y = (int16_t)-motion->left_y;
            break;
        case MOTION_FAST_PULSE:
            motion->size = (int16_t)wave(elapsed, 700, 2, 0);
            break;
        case MOTION_SHY: {
                int16_t inward = (int16_t)LV_ABS(wave(elapsed, 1500, 2, 0));
                motion->left_x = inward;
                motion->right_x = (int16_t)-inward;
                break;
            }
        case MOTION_DROOP:
            motion->pair_y = (int16_t)LV_ABS(wave(elapsed, 2200, 1, 0));
            break;
        case MOTION_SLEEPY:
            motion->left_y = (int16_t)wave(elapsed, 2100, 1, 0);
            motion->right_y = (int16_t)-motion->left_y;
            break;
        case MOTION_SLEEP:
            motion->pair_y = (int16_t)wave(elapsed, 2600, 1, 0);
            break;
        case MOTION_JITTER:
            motion->pair_x = (int16_t)(((elapsed / 90U) & 1U) ? 1 : -1);
            motion->left_y = (int16_t)-motion->pair_x;
            motion->right_y = motion->pair_x;
            break;
        case MOTION_SWAY:
            motion->pair_x = (int16_t)wave(elapsed, 1300, 3, 0);
            break;
        case MOTION_TREMBLE:
            motion->pair_x = (int16_t)(((elapsed / 70U) & 1U) ? 2 : -2);
            motion->left_y = (int16_t)wave(elapsed, 240, 1, 0);
            motion->right_y = (int16_t)-motion->left_y;
            break;
        case MOTION_PANIC:
            motion->pair_x = (int16_t)(((elapsed / 80U) & 1U) ? 1 : -1);
            motion->size = (int16_t)LV_ABS(wave(elapsed, 430, 3, 0));
            break;
        case MOTION_STILL:
        default:
            break;
    }
}

static void clock_hand_endpoint(int32_t hour, int32_t minute, int32_t length,
                                int32_t * x, int32_t * y)
{
    int32_t angle = ((hour % 12) * 30) + (minute / 2);
    *x = SCREEN_CENTER + ((length * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT);
    *y = SCREEN_CENTER - ((length * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT);
}

static void clock_minute_endpoint(int32_t minute, int32_t length,
                                  int32_t * x, int32_t * y)
{
    int32_t angle = minute * 6;
    *x = SCREEN_CENTER + ((length * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT);
    *y = SCREEN_CENTER - ((length * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT);
}

static void init_clock_particles(void)
{
    const expression_def_t * expression = &expressions[state.current];
    motion_offset_t motion;
    calculate_motion(&motion);

    int32_t pair_x = expression->gaze_x + state.touch_gaze_x + motion.pair_x;
    int32_t pair_y = expression->gaze_y + state.touch_gaze_y + motion.pair_y;
    int32_t eye_center_x[2] = {
        SCREEN_CENTER - EYE_OFFSET_X + expression->left.x + pair_x + motion.left_x,
        SCREEN_CENTER + EYE_OFFSET_X + expression->right.x + pair_x + motion.right_x
    };
    int32_t eye_center_y[2] = {
        EYE_CENTER_Y + expression->left.y + pair_y + motion.left_y,
        EYE_CENTER_Y + expression->right.y + pair_y + motion.right_y
    };
    int32_t eye_width[2] = {
        LV_MAX(expression->left.width + motion.size * 2, 8),
        LV_MAX(expression->right.width + motion.size * 2, 8)
    };
    int32_t eye_height[2] = {
        LV_MAX(expression->left.height + motion.size, 6),
        LV_MAX(expression->right.height + motion.size, 6)
    };

    int32_t minute_tip_x;
    int32_t minute_tip_y;
    int32_t hour_tip_x;
    int32_t hour_tip_y;
    clock_minute_endpoint(state.clock_minute, 57, &minute_tip_x, &minute_tip_y);
    clock_hand_endpoint(state.clock_hour, state.clock_minute, 38, &hour_tip_x, &hour_tip_y);

    for(int32_t i = 0; i < CLOCK_PARTICLE_COUNT; ++i) {
        clock_particle_t * particle = &state.clock_particles[i];
        int32_t source = i & 1;

        if(i < 78) {
            int32_t lane = i / 2;
            int32_t col = lane % 7;
            int32_t row = (lane / 7) % 6;
            int32_t nx = (col - 3) * eye_width[source] / 7;
            int32_t ny = (row - 2) * eye_height[source] / 6;
            particle->start_x = (int16_t)(eye_center_x[source] + nx);
            particle->start_y = (int16_t)(eye_center_y[source] + ny);
        }
        else {
            int32_t angle = (i - 78) * 20;
            int32_t radius = 16 + ((i % 5) * 4);
            particle->start_x = (int16_t)(SCREEN_CENTER +
                                          ((radius * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT));
            particle->start_y = (int16_t)(68 -
                                          ((radius * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT));
        }

        if(i < 48) {
            int32_t angle = i * 360 / 48;
            int32_t radius = 64 + ((i % 3) - 1);
            particle->target_x = (int16_t)(SCREEN_CENTER +
                                           ((radius * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT));
            particle->target_y = (int16_t)(SCREEN_CENTER -
                                           ((radius * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT));
            particle->size = 4;
        }
        else if(i < 60) {
            int32_t angle = (i - 48) * 30;
            int32_t radius = 53;
            particle->target_x = (int16_t)(SCREEN_CENTER +
                                           ((radius * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT));
            particle->target_y = (int16_t)(SCREEN_CENTER -
                                           ((radius * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT));
            particle->size = ((i - 48) % 3 == 0) ? 7 : 5;
        }
        else if(i < 78) {
            int32_t step = i - 60;
            particle->target_x = (int16_t)lerp_i32(SCREEN_CENTER, minute_tip_x, (step + 1) * 1000 / 18);
            particle->target_y = (int16_t)lerp_i32(SCREEN_CENTER, minute_tip_y, (step + 1) * 1000 / 18);
            particle->size = 5;
        }
        else if(i < 92) {
            int32_t step = i - 78;
            particle->target_x = (int16_t)lerp_i32(SCREEN_CENTER, hour_tip_x, (step + 1) * 1000 / 14);
            particle->target_y = (int16_t)lerp_i32(SCREEN_CENTER, hour_tip_y, (step + 1) * 1000 / 14);
            particle->size = 6;
        }
        else {
            int32_t angle = (i - 92) * 90;
            particle->target_x = (int16_t)(SCREEN_CENTER +
                                           ((5 * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT));
            particle->target_y = (int16_t)(SCREEN_CENTER -
                                           ((5 * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT));
            particle->size = 6;
        }

        particle->delay = (uint8_t)((i * 37) % 160);
    }
}

static void draw_fill_capsule(lv_layer_t * layer, int32_t center_x, int32_t center_y,
                              int32_t width, int32_t height, lv_color_t color,
                              lv_opa_t opacity, int32_t radius)
{
    lv_area_t area;
    area.x1 = center_x - width / 2;
    area.y1 = center_y - height / 2;
    area.x2 = area.x1 + width - 1;
    area.y2 = area.y1 + height - 1;

    lv_draw_fill_dsc_t fill;
    lv_draw_fill_dsc_init(&fill);
    fill.color = color;
    fill.opa = opacity;
    fill.radius = radius;
    lv_draw_fill(layer, &fill, &area);
}

static void draw_line_segment(lv_layer_t * layer, int32_t x1, int32_t y1,
                              int32_t x2, int32_t y2, int32_t width,
                              lv_color_t color, lv_opa_t opacity)
{
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.p1.x = x1;
    line.p1.y = y1;
    line.p2.x = x2;
    line.p2.y = y2;
    line.color = color;
    line.width = width;
    line.opa = opacity;
    line.round_start = 1;
    line.round_end = 1;
    lv_draw_line(layer, &line);
}

static void draw_triangle_shape(lv_layer_t * layer,
                                int32_t x1, int32_t y1,
                                int32_t x2, int32_t y2,
                                int32_t x3, int32_t y3,
                                lv_color_t color, lv_opa_t opacity)
{
    lv_draw_triangle_dsc_t triangle;
    lv_draw_triangle_dsc_init(&triangle);
    triangle.p[0].x = x1;
    triangle.p[0].y = y1;
    triangle.p[1].x = x2;
    triangle.p[1].y = y2;
    triangle.p[2].x = x3;
    triangle.p[2].y = y3;
    triangle.color = color;
    triangle.opa = opacity;
    lv_draw_triangle(layer, &triangle);
}

static void draw_capsule(lv_layer_t * layer, int32_t center_x, int32_t center_y,
                         int32_t width, int32_t height, int32_t tilt, lv_opa_t opacity)
{
    width = LV_MAX(width, 4);
    height = LV_MAX(height, 2);

    if(tilt == 0) {
        draw_fill_capsule(layer, center_x, center_y, width, height,
                          lv_color_white(), opacity, LV_RADIUS_CIRCLE);
        return;
    }

    int32_t half_line = LV_MAX((width - height) / 2, 1);
    draw_line_segment(layer,
                      center_x - half_line, center_y - tilt / 2,
                      center_x + half_line, center_y + tilt / 2,
                      height, lv_color_white(), opacity);
}

static void draw_arc(lv_layer_t * layer, int32_t center_x, int32_t center_y,
                     int32_t radius, int32_t width, int32_t start, int32_t end,
                     lv_color_t color, lv_opa_t opacity)
{
    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = color;
    arc.opa = opacity;
    arc.center.x = center_x;
    arc.center.y = center_y;
    arc.radius = (uint16_t)LV_MAX(radius, 2);
    arc.width = LV_MAX(width, 1);
    arc.start_angle = start;
    arc.end_angle = end;
    arc.rounded = 1;
    lv_draw_arc(layer, &arc);
}

static void draw_heart(lv_layer_t * layer, int32_t x, int32_t y,
                       int32_t size, lv_opa_t opacity)
{
    int32_t radius = LV_MAX(size / 4, 2);
    draw_fill_capsule(layer, x - radius + 2, y - radius / 2, radius * 2, radius * 2,
                      lv_color_white(), opacity, LV_RADIUS_CIRCLE);
    draw_fill_capsule(layer, x + radius - 2, y - radius / 2, radius * 2, radius * 2,
                      lv_color_white(), opacity, LV_RADIUS_CIRCLE);
    draw_fill_capsule(layer, x, y, radius, radius,
                      lv_color_white(), opacity, LV_RADIUS_CIRCLE);
    draw_triangle_shape(layer,
                        x - size / 2, y,
                        x + size / 2, y,
                        x, y + size / 2,
                        lv_color_white(), opacity);
}

static void draw_sparkle(lv_layer_t * layer, int32_t x, int32_t y,
                         int32_t size, lv_opa_t opacity)
{
    draw_line_segment(layer, x, y - size, x, y + size, 2, lv_color_white(), opacity);
    draw_line_segment(layer, x - size, y, x + size, y, 2, lv_color_white(), opacity);
    draw_line_segment(layer, x - size / 2, y - size / 2,
                      x + size / 2, y + size / 2, 1, lv_color_white(), opacity);
    draw_line_segment(layer, x + size / 2, y - size / 2,
                      x - size / 2, y + size / 2, 1, lv_color_white(), opacity);
}

static void draw_drop(lv_layer_t * layer, int32_t x, int32_t y,
                      int32_t size, lv_opa_t opacity)
{
    draw_triangle_shape(layer,
                        x, y - size,
                        x - size / 2, y,
                        x + size / 2, y,
                        lv_color_white(), opacity);
    draw_fill_capsule(layer, x, y + size / 4, size, size,
                      lv_color_white(), opacity, LV_RADIUS_CIRCLE);
}

static void draw_eye_shape(lv_layer_t * layer, const eye_visual_t * eye,
                           int32_t center_x, int32_t center_y,
                           int32_t width, int32_t height, lv_opa_t opacity)
{
    int32_t radius = LV_MAX(width / 2, 8);

    switch(eye->shape) {
        case EYE_SHAPE_CAPSULE:
            draw_capsule(layer, center_x, center_y, width, height, eye->tilt, opacity);
            break;
        case EYE_SHAPE_ARC_UP:
            draw_arc(layer, center_x, center_y + radius / 2, radius,
                     LV_MAX(height, 3), 198, 342, lv_color_white(), opacity);
            break;
        case EYE_SHAPE_ARC_DOWN:
            draw_arc(layer, center_x, center_y - radius / 2, radius,
                     LV_MAX(height, 3), 18, 162, lv_color_white(), opacity);
            break;
        case EYE_SHAPE_RING: {
                int32_t diameter = LV_MAX(LV_MIN(width, height), 14);
                draw_fill_capsule(layer, center_x, center_y, diameter, diameter,
                                  lv_color_white(), opacity, LV_RADIUS_CIRCLE);
                draw_fill_capsule(layer, center_x, center_y,
                                  LV_MAX(diameter - 10, 4), LV_MAX(diameter - 10, 4),
                                  lv_color_black(), LV_OPA_COVER, LV_RADIUS_CIRCLE);
                break;
            }
        case EYE_SHAPE_X:
            draw_line_segment(layer, center_x - width / 2, center_y - height / 2,
                              center_x + width / 2, center_y + height / 2,
                              5, lv_color_white(), opacity);
            draw_line_segment(layer, center_x + width / 2, center_y - height / 2,
                              center_x - width / 2, center_y + height / 2,
                              5, lv_color_white(), opacity);
            break;
        case EYE_SHAPE_HEART:
            draw_heart(layer, center_x, center_y - 3, LV_MIN(width, height), opacity);
            break;
        case EYE_SHAPE_STAR:
            draw_sparkle(layer, center_x, center_y, LV_MIN(width, height) / 2, opacity);
            draw_fill_capsule(layer, center_x, center_y, 7, 7,
                              lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            break;
        case EYE_SHAPE_SQUARE:
            draw_fill_capsule(layer, center_x, center_y, width, height,
                              lv_color_white(), opacity, 3);
            break;
        case EYE_SHAPE_DIAMOND:
            draw_triangle_shape(layer,
                                center_x, center_y - height / 2,
                                center_x - width / 2, center_y,
                                center_x + width / 2, center_y,
                                lv_color_white(), opacity);
            draw_triangle_shape(layer,
                                center_x, center_y + height / 2,
                                center_x - width / 2, center_y,
                                center_x + width / 2, center_y,
                                lv_color_white(), opacity);
            break;
        case EYE_SHAPE_SPIRAL:
            draw_arc(layer, center_x, center_y, width / 2, 4,
                     20, 325, lv_color_white(), opacity);
            draw_arc(layer, center_x, center_y, width / 4, 4,
                     190, 500, lv_color_white(), opacity);
            draw_fill_capsule(layer, center_x + 2, center_y + 1, 5, 5,
                              lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            break;
        default:
            break;
    }
}

static void draw_eye(lv_layer_t * layer, const eye_visual_t * eye,
                     int32_t center_x, int32_t center_y, int32_t size_delta)
{
    int32_t width = LV_MAX(eye->width + size_delta * 2, 4);
    int32_t height = LV_MAX(eye->height + size_delta, 2);
    lv_opa_t shape_opa = (lv_opa_t)(LV_OPA_COVER * (100 - state.blink) / 100);
    lv_opa_t closed_opa = (lv_opa_t)(LV_OPA_COVER * state.blink / 100);

    if(shape_opa > LV_OPA_TRANSP) {
        draw_eye_shape(layer, eye, center_x, center_y, width, height, shape_opa);
    }
    if(closed_opa > LV_OPA_TRANSP) {
        draw_capsule(layer, center_x, center_y, LV_MAX(width - 4, 8), 4, 0, closed_opa);
    }
}

static void draw_hash(lv_layer_t * layer, int32_t x, int32_t y, lv_opa_t opacity)
{
    draw_line_segment(layer, x - 6, y - 10, x - 8, y + 10, 3, lv_color_white(), opacity);
    draw_line_segment(layer, x + 7, y - 10, x + 5, y + 10, 3, lv_color_white(), opacity);
    draw_line_segment(layer, x - 13, y - 4, x + 13, y - 5, 3, lv_color_white(), opacity);
    draw_line_segment(layer, x - 13, y + 5, x + 13, y + 4, 3, lv_color_white(), opacity);
}

static void draw_question(lv_layer_t * layer, int32_t x, int32_t y, lv_opa_t opacity)
{
    draw_arc(layer, x, y - 4, 8, 3, 205, 450, lv_color_white(), opacity);
    draw_line_segment(layer, x, y + 4, x - 3, y + 9, 3, lv_color_white(), opacity);
    draw_fill_capsule(layer, x - 3, y + 15, 4, 4,
                      lv_color_white(), opacity, LV_RADIUS_CIRCLE);
}

static void draw_exclamation(lv_layer_t * layer, int32_t x, int32_t y, lv_opa_t opacity)
{
    draw_line_segment(layer, x, y - 11, x, y + 4, 4, lv_color_white(), opacity);
    draw_fill_capsule(layer, x, y + 12, 5, 5,
                      lv_color_white(), opacity, LV_RADIUS_CIRCLE);
}

static void draw_z(lv_layer_t * layer, int32_t x, int32_t y,
                   int32_t size, lv_opa_t opacity)
{
    draw_line_segment(layer, x - size / 2, y - size / 2,
                      x + size / 2, y - size / 2, 2, lv_color_white(), opacity);
    draw_line_segment(layer, x + size / 2, y - size / 2,
                      x - size / 2, y + size / 2, 2, lv_color_white(), opacity);
    draw_line_segment(layer, x - size / 2, y + size / 2,
                      x + size / 2, y + size / 2, 2, lv_color_white(), opacity);
}

static void draw_accessory(lv_layer_t * layer, accessory_effect_t effect, uint32_t elapsed)
{
    lv_opa_t opacity = (lv_opa_t)(220 * (100 - state.blink) / 100);
    if(opacity < 8 || effect == EFFECT_NONE) return;

    int32_t bob = wave(elapsed, 1200, 2, 0);
    int32_t pulse = 5 + LV_ABS(wave(elapsed, 800, 2, 0));

    switch(effect) {
        case EFFECT_SOUND:
            draw_arc(layer, 190, 103, 7, 2, 270, 450, lv_color_white(), opacity);
            draw_arc(layer, 190, 103, 12, 2, 270, 450, lv_color_white(), opacity);
            draw_arc(layer, 190, 103, 17, 2, 270, 450, lv_color_white(), opacity);
            break;
        case EFFECT_DOTS:
            draw_fill_capsule(layer, 108, 56 + bob, 5, 5, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            draw_fill_capsule(layer, 120, 56 + bob, 5, 5, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            draw_fill_capsule(layer, 132, 56 + bob, 5, 5, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            break;
        case EFFECT_CHECK:
            draw_line_segment(layer, 109, 57, 117, 65, 4, lv_color_white(), opacity);
            draw_line_segment(layer, 117, 65, 133, 47, 4, lv_color_white(), opacity);
            break;
        case EFFECT_HASH:
            draw_hash(layer, 120, 54 + bob, opacity);
            break;
        case EFFECT_SPARKLES:
            draw_sparkle(layer, 52, 61 + bob, pulse, opacity);
            draw_sparkle(layer, 191, 55 - bob, LV_MAX(pulse - 2, 3), opacity);
            break;
        case EFFECT_QUESTION:
            draw_question(layer, 194, 59 + bob, opacity);
            break;
        case EFFECT_SWEAT:
            draw_drop(layer, 198, 62 + bob, 10, opacity);
            break;
        case EFFECT_EXCLAMATION:
            draw_exclamation(layer, 194, 59 + bob, opacity);
            break;
        case EFFECT_HEARTS:
            draw_heart(layer, 52, 62 + bob, 13, opacity);
            draw_heart(layer, 188, 57 - bob, 10, opacity);
            break;
        case EFFECT_BREATH: {
                int32_t drift = (int32_t)((elapsed % 1400U) * 24U / 1400U);
                draw_fill_capsule(layer, 169 + drift, 148 + bob, 6, 6,
                                  lv_color_white(), (lv_opa_t)(opacity / 2), LV_RADIUS_CIRCLE);
                draw_fill_capsule(layer, 178 + drift, 144 + bob, 14, 12,
                                  lv_color_white(), opacity, LV_RADIUS_CIRCLE);
                draw_fill_capsule(layer, 187 + drift, 140 - bob, 13, 13,
                                  lv_color_white(), opacity, LV_RADIUS_CIRCLE);
                draw_fill_capsule(layer, 196 + drift, 145, 11, 10,
                                  lv_color_white(), (lv_opa_t)(opacity * 3 / 4), LV_RADIUS_CIRCLE);
                break;
            }
        case EFFECT_Z:
            draw_z(layer, 190, 64 + bob, 13, opacity);
            break;
        case EFFECT_ZZZ:
            draw_z(layer, 177, 76, 9, (lv_opa_t)(opacity / 2));
            draw_z(layer, 191, 62, 12, (lv_opa_t)(opacity * 3 / 4));
            draw_z(layer, 207, 47, 15, opacity);
            break;
        case EFFECT_TEARS: {
                int32_t fall = (int32_t)((elapsed % 800U) * 14U / 800U);
                draw_drop(layer, 78, 131 + fall, 8, opacity);
                draw_drop(layer, 162, 131 + ((fall + 7) % 14), 8, opacity);
                break;
            }
        case EFFECT_ONE_TEAR: {
                int32_t fall = (int32_t)((elapsed % 950U) * 12U / 950U);
                draw_drop(layer, 164, 130 + fall, 8, opacity);
                break;
            }
        case EFFECT_SHINE:
            draw_sparkle(layer, 194, 60 + bob, pulse, opacity);
            break;
        case EFFECT_CONFETTI:
            draw_line_segment(layer, 46, 59, 52, 65, 2, lv_color_white(), opacity);
            draw_line_segment(layer, 68, 45, 66, 53, 2, lv_color_white(), opacity);
            draw_line_segment(layer, 176, 50, 183, 45, 2, lv_color_white(), opacity);
            draw_line_segment(layer, 195, 69, 201, 74, 2, lv_color_white(), opacity);
            draw_fill_capsule(layer, 53, 79 + bob, 4, 4, lv_color_white(), opacity, 1);
            draw_fill_capsule(layer, 189, 84 - bob, 4, 4, lv_color_white(), opacity, 1);
            break;
        case EFFECT_STEAM:
            draw_fill_capsule(layer, 45, 111 + bob, 12, 10, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            draw_fill_capsule(layer, 34, 106 + bob, 9, 8, lv_color_white(), (lv_opa_t)(opacity * 3 / 4), LV_RADIUS_CIRCLE);
            draw_fill_capsule(layer, 195, 111 + bob, 12, 10, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            draw_fill_capsule(layer, 206, 106 + bob, 9, 8, lv_color_white(), (lv_opa_t)(opacity * 3 / 4), LV_RADIUS_CIRCLE);
            break;
        case EFFECT_STRESS:
            draw_line_segment(layer, 54, 62, 45, 53, 3, lv_color_white(), opacity);
            draw_line_segment(layer, 63, 56, 59, 44, 3, lv_color_white(), opacity);
            draw_line_segment(layer, 178, 56, 182, 44, 3, lv_color_white(), opacity);
            draw_line_segment(layer, 187, 62, 196, 53, 3, lv_color_white(), opacity);
            break;
        case EFFECT_BURST:
            for(int32_t i = 0; i < 8; ++i) {
                int32_t angle = i * 45;
                int32_t x1 = 120 + ((34 * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT);
                int32_t y1 = 79 + ((24 * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT);
                int32_t x2 = 120 + ((44 * lv_trigo_sin(angle + 90)) >> LV_TRIGO_SHIFT);
                int32_t y2 = 79 + ((34 * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT);
                draw_line_segment(layer, x1, y1, x2, y2, 2, lv_color_white(), opacity);
            }
            break;
        case EFFECT_SCAN: {
                int32_t y = 78 + (int32_t)((elapsed % 1100U) * 50U / 1100U);
                draw_line_segment(layer, 54, y, 186, y, 2,
                                  lv_color_white(), (lv_opa_t)(opacity / 2));
                break;
            }
        case EFFECT_SNOW:
            draw_line_segment(layer, 194, 50, 194, 72, 2, lv_color_white(), opacity);
            draw_line_segment(layer, 184, 56, 204, 67, 2, lv_color_white(), opacity);
            draw_line_segment(layer, 184, 67, 204, 56, 2, lv_color_white(), opacity);
            break;
        case EFFECT_STARS:
            draw_sparkle(layer, 50, 62 + bob, 6, opacity);
            draw_sparkle(layer, 190, 56 - bob, 8, opacity);
            draw_sparkle(layer, 200, 86 + bob, 4, (lv_opa_t)(opacity * 3 / 4));
            break;
        case EFFECT_RAGE:
            draw_hash(layer, 74, 55 + bob, opacity);
            draw_hash(layer, 168, 55 - bob, opacity);
            draw_fill_capsule(layer, 42, 111, 11, 9, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            draw_fill_capsule(layer, 198, 111, 11, 9, lv_color_white(), opacity, LV_RADIUS_CIRCLE);
            break;
        case EFFECT_NONE:
        default:
            break;
    }
}

static void draw_clock_particles(lv_layer_t * layer, uint32_t elapsed)
{
    int32_t morph_progress;
    int32_t clock_opacity;

    if(elapsed < CLOCK_MORPH_MS) {
        morph_progress = ease_out_cubic_i32((int32_t)(elapsed * 1000U / CLOCK_MORPH_MS));
        clock_opacity = (int32_t)(80 + (elapsed * 175U / CLOCK_MORPH_MS));
    }
    else if(elapsed < CLOCK_MORPH_MS + CLOCK_HOLD_MS) {
        morph_progress = 1000;
        clock_opacity = 255;
    }
    else {
        uint32_t return_elapsed = elapsed - CLOCK_MORPH_MS - CLOCK_HOLD_MS;
        int32_t return_progress = ease_out_cubic_i32((int32_t)(return_elapsed * 1000U / CLOCK_RETURN_MS));
        morph_progress = 1000 - return_progress;
        clock_opacity = 255 - (int32_t)(return_progress * 175 / 1000);
    }

    clock_opacity = clamp_i32(clock_opacity, 0, 255);

    if(morph_progress > 900) {
        int32_t face_opa = (morph_progress - 900) * clock_opacity / 100;
        draw_arc(layer, SCREEN_CENTER, SCREEN_CENTER, 68, 2, 0, 360,
                 lv_color_white(), (lv_opa_t)clamp_i32(face_opa, 0, 140));
    }

    for(int32_t i = 0; i < CLOCK_PARTICLE_COUNT; ++i) {
        const clock_particle_t * particle = &state.clock_particles[i];
        int32_t local = clamp_i32((morph_progress * 1160 / 1000) - particle->delay, 0, 1000);
        int32_t p = ease_out_cubic_i32(local);
        int32_t x = lerp_i32(particle->start_x, particle->target_x, p);
        int32_t y = lerp_i32(particle->start_y, particle->target_y, p);
        int32_t size = particle->size;

        if(elapsed < CLOCK_MORPH_MS && local < 1000) {
            size += (int32_t)((1000 - local) / 280);
        }

        draw_fill_capsule(layer, x, y, size, size, lv_color_white(),
                          (lv_opa_t)clock_opacity, 1);
    }

    if(morph_progress > 930) {
        int32_t hand_opa = clamp_i32((morph_progress - 930) * clock_opacity / 70, 0, 255);
        int32_t minute_x;
        int32_t minute_y;
        int32_t hour_x;
        int32_t hour_y;

        clock_minute_endpoint(state.clock_minute, 57, &minute_x, &minute_y);
        clock_hand_endpoint(state.clock_hour, state.clock_minute, 38, &hour_x, &hour_y);
        draw_line_segment(layer, SCREEN_CENTER, SCREEN_CENTER, minute_x, minute_y,
                          3, lv_color_white(), (lv_opa_t)hand_opa);
        draw_line_segment(layer, SCREEN_CENTER, SCREEN_CENTER, hour_x, hour_y,
                          5, lv_color_white(), (lv_opa_t)hand_opa);
        draw_fill_capsule(layer, SCREEN_CENTER, SCREEN_CENTER, 9, 9,
                          lv_color_white(), (lv_opa_t)hand_opa, LV_RADIUS_CIRCLE);
    }
}

static void draw_event_cb(lv_event_t * event)
{
    const expression_def_t * expression = &expressions[state.current];
    motion_offset_t motion;
    calculate_motion(&motion);

    int32_t pair_x = expression->gaze_x + state.touch_gaze_x + motion.pair_x;
    int32_t pair_y = expression->gaze_y + state.touch_gaze_y + motion.pair_y;
    int32_t left_x = SCREEN_CENTER - EYE_OFFSET_X + expression->left.x + pair_x + motion.left_x;
    int32_t right_x = SCREEN_CENTER + EYE_OFFSET_X + expression->right.x + pair_x + motion.right_x;
    int32_t left_y = EYE_CENTER_Y + expression->left.y + pair_y + motion.left_y;
    int32_t right_y = EYE_CENTER_Y + expression->right.y + pair_y + motion.right_y;

    lv_layer_t * layer = lv_event_get_layer(event);
    if(state.clock_active) {
        uint32_t elapsed = lv_tick_elaps(state.clock_started);
        if(elapsed < 160U) {
            lv_opa_t fade = (lv_opa_t)(LV_OPA_COVER * (160U - elapsed) / 160U);
            draw_eye(layer, &expression->left, left_x, left_y, motion.size);
            draw_eye(layer, &expression->right, right_x, right_y, motion.size);
            draw_accessory(layer, expression->effect, lv_tick_elaps(state.motion_started));
            draw_fill_capsule(layer, SCREEN_CENTER, SCREEN_CENTER, SCREEN_SIZE, SCREEN_SIZE,
                              lv_color_black(), (lv_opa_t)(LV_OPA_COVER - fade), LV_RADIUS_CIRCLE);
        }

        draw_clock_particles(layer, elapsed);
        return;
    }

    draw_eye(layer, &expression->left, left_x, left_y, motion.size);
    draw_eye(layer, &expression->right, right_x, right_y, motion.size);
    draw_accessory(layer, expression->effect, lv_tick_elaps(state.motion_started));
}

static void set_blink(void * var, int32_t value)
{
    LV_UNUSED(var);
    state.blink = value;
    invalidate_eyes();
}

static void set_touch_gaze_x(void * var, int32_t value)
{
    LV_UNUSED(var);
    state.touch_gaze_x = value;
    invalidate_eyes();
}

static void set_touch_gaze_y(void * var, int32_t value)
{
    LV_UNUSED(var);
    state.touch_gaze_y = value;
    invalidate_eyes();
}

static void animate_value(lv_anim_exec_xcb_t exec_cb, int32_t from, int32_t to,
                          uint32_t duration)
{
    lv_anim_delete(NULL, exec_cb);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, NULL);
    lv_anim_set_exec_cb(&anim, exec_cb);
    lv_anim_set_values(&anim, from, to);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void transition_opened_cb(lv_anim_t * anim)
{
    LV_UNUSED(anim);
    state.transitioning = false;
}

static void transition_closed_cb(lv_anim_t * anim)
{
    LV_UNUSED(anim);
    state.current = state.pending;
    state.motion_started = lv_tick_get();
    invalidate_eyes();

    lv_anim_t open;
    lv_anim_init(&open);
    lv_anim_set_var(&open, NULL);
    lv_anim_set_exec_cb(&open, set_blink);
    lv_anim_set_values(&open, 100, 0);
    lv_anim_set_duration(&open, 220);
    lv_anim_set_path_cb(&open, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&open, transition_opened_cb);
    lv_anim_start(&open);
}

static void request_expression(nomi_expression_t expression)
{
    if(expression < 0 || expression >= NOMI_EXPRESSION_COUNT) return;
    if(expression == state.current && !state.transitioning) return;

    state.pending = expression;
    if(state.transitioning) return;

    state.transitioning = true;
    lv_anim_delete(NULL, set_blink);

    lv_anim_t close;
    lv_anim_init(&close);
    lv_anim_set_var(&close, NULL);
    lv_anim_set_exec_cb(&close, set_blink);
    lv_anim_set_values(&close, state.blink, 100);
    lv_anim_set_duration(&close, 130);
    lv_anim_set_path_cb(&close, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&close, transition_closed_cb);
    lv_anim_start(&close);
}

static void blink(void)
{
    if(state.transitioning) return;
    lv_anim_delete(NULL, set_blink);

    uint32_t close_ms = 85;
    uint32_t open_ms = 130;
    if(state.current == NOMI_EXPRESSION_RELAXED) {
        close_ms = 190;
        open_ms = 260;
    }
    else if(state.current == NOMI_EXPRESSION_SLEEPY) {
        close_ms = 260;
        open_ms = 340;
    }

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, NULL);
    lv_anim_set_exec_cb(&anim, set_blink);
    lv_anim_set_values(&anim, state.blink, 100);
    lv_anim_set_duration(&anim, close_ms);
    lv_anim_set_reverse_duration(&anim, open_ms);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

static void update_gaze_from_pointer(void)
{
    lv_indev_t * indev = lv_indev_active();
    if(!indev) return;

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    state.touch_gaze_x = clamp_i32((point.x - SCREEN_CENTER) / 7, -10, 10);
    state.touch_gaze_y = clamp_i32((point.y - EYE_CENTER_Y) / 9, -6, 6);
    invalidate_eyes();
}

static void input_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        state.pressed = true;
        update_gaze_from_pointer();
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        state.pressed = false;
        animate_value(set_touch_gaze_x, state.touch_gaze_x, 0, 280);
        animate_value(set_touch_gaze_y, state.touch_gaze_y, 0, 280);
    }
    else if(code == LV_EVENT_CLICKED) {
        blink();
    }
    else if(code == LV_EVENT_DOUBLE_CLICKED || code == LV_EVENT_LONG_PRESSED) {
        request_expression(NOMI_EXPRESSION_HAPPY);
        if(state.carousel_timer) lv_timer_reset(state.carousel_timer);
    }
}

static void carousel_timer_cb(lv_timer_t * timer)
{
    if(state.pressed || state.transitioning) {
        lv_timer_reset(timer);
        return;
    }

    nomi_expression_t next = (nomi_expression_t)((state.current + 1) % NOMI_EXPRESSION_COUNT);
    request_expression(next);
}

static void idle_blink_cb(lv_timer_t * timer)
{
    if(!state.pressed && !state.transitioning && expressions[state.current].auto_blink) blink();
    lv_timer_set_period(timer, lv_rand(2400, 4200));
}

static void motion_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    invalidate_eyes();
}

void nomi_eyes_create(lv_obj_t * parent)
{
    lv_memzero(&state, sizeof(state));
    state.current = NOMI_EXPRESSION_IDLE;
    state.pending = NOMI_EXPRESSION_IDLE;
    state.motion_started = lv_tick_get();

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x181818), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * face = lv_obj_create(parent);
    lv_obj_remove_style_all(face);
    lv_obj_set_size(face, SCREEN_SIZE, SCREEN_SIZE);
    lv_obj_center(face);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(face, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);
    lv_obj_add_flag(face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(face, LV_OBJ_FLAG_SCROLLABLE);

    state.canvas = lv_obj_create(face);
    lv_obj_remove_style_all(state.canvas);
    lv_obj_set_size(state.canvas, SCREEN_SIZE, SCREEN_SIZE);
    lv_obj_center(state.canvas);
    lv_obj_add_flag(state.canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(state.canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(state.canvas, draw_event_cb, LV_EVENT_DRAW_MAIN_END, NULL);
    lv_obj_add_event_cb(state.canvas, input_event_cb, LV_EVENT_ALL, NULL);

    state.carousel_timer = lv_timer_create(carousel_timer_cb, DEFAULT_CAROUSEL_PERIOD_MS, NULL);
    lv_timer_create(motion_timer_cb, MOTION_PERIOD_MS, NULL);

    lv_timer_t * idle_timer = lv_timer_create(idle_blink_cb, 3000, NULL);
    lv_timer_set_period(idle_timer, lv_rand(2400, 3800));
}

void nomi_eyes_set_carousel_period(uint32_t period_ms)
{
    if(!state.carousel_timer) return;
    lv_timer_set_period(state.carousel_timer, LV_MAX(period_ms, 450U));
    lv_timer_reset(state.carousel_timer);
}

nomi_expression_t nomi_eyes_current_expression(void)
{
    return state.current;
}

const char * nomi_eyes_expression_name(nomi_expression_t expression)
{
    if(expression < 0 || expression >= NOMI_EXPRESSION_COUNT) return "Unknown";
    return expressions[expression].name;
}
