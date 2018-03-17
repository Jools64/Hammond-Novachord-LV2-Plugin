// attack should be 50 and release should be 500

typedef struct Limiter
{
    f64 envelope, attack, release;
    u32 sampleRate;
} Limiter;

void Limiter_init(Limiter* limiter, f64 attackMs, f64 releaseMs, u32 sampleRate)
{
    Limiter l;
    l.attack = pow(0.01, 1.0 / (attackMs * sampleRate * 0.001));
    l.release = pow(0.01, 1.0 / (releaseMs * sampleRate * 0.001));
    l.sampleRate = sampleRate;
    l.envelope = 0;
    *limiter = l;
}

void Limiter_followEnvelop(Limiter* limiter, u32 count, const float* samples)
{
    while(count--)
    {
        f64 v = fabs(*samples);
        samples += 1; 
        if(v > limiter->envelope)
            limiter->envelope = limiter->attack * (limiter->envelope - v) + v;
        else
            limiter->envelope = limiter->release * (limiter->envelope - v) + v;
    }
}

void Limiter_process(Limiter* limiter, u32 count, float* samples)
{
    while(count--)
    {
        float v = *samples;
        Limiter_followEnvelop(limiter, 1, &v);
        if(limiter->envelope > 1)
        {
            *samples = (*samples / limiter->envelope) * 0.98;
        }
        samples += 1;
    }
}
