extern int _snd_ev_flag;
extern void _SsVmFlush(void);

void SpuVmDamperStep(void) {
    if (_snd_ev_flag == 1) return;
    _snd_ev_flag = 1;
    _SsVmFlush();
    _snd_ev_flag = 0;
}
