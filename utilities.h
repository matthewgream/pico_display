
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#ifdef DEBUG
#define debug(__info,...) printf ("# " __info "\n",##__VA_ARGS__)
#else
#define debug(__info,...)
#endif

// ------------------------------------------------------------------------------------------------------------------------

static bool __exceeded (uint32_t * const counter, const uint32_t boundary, const uint32_t interval) {
    if (boundary == 0 || interval == 0)
        return false;
    if (*counter > boundary) {
        *counter -= boundary;
        return true;
    } else if ((boundary - *counter) <= interval) {
        *counter = (interval - (boundary - *counter));
        return true;
    } else {
        *counter += interval;
        return false;
    }
}

static bool __isspaced (uint32_t * const last, const uint32_t time) {
    uint32_t curr = (uint32_t) (time_us_64 () / 1000);
    if ((curr - *last) < time)
        return false;
    *last = curr;
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------

#define __float_smooth_n 64

typedef struct {
    float val;
    float buf [__float_smooth_n];
    int   cnt;
    int   idx;
    float sum;
} __float_smooth_t;

static float __float_smooth (const float val, __float_smooth_t * const smooth) {
    if (smooth->cnt == __float_smooth_n)
        smooth->sum -= smooth->buf [smooth->idx];
    else
        smooth->cnt ++;
    smooth->sum += val;
    smooth->buf [smooth->idx] = val;
    smooth->idx = (smooth->idx + 1) & (__float_smooth_n - 1);
    return smooth->sum / (float) smooth->cnt;
}

static inline bool __float_equals_one_place (const float a, const float b) {
    return roundf (a * 10.0f) == roundf (b * 10.0f);
}

static inline float __float_clamp (const float v, const float l, const float h) {
    return v < l ? l : (v > h ? h : v);
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

