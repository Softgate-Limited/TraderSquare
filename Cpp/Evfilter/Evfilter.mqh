#import "Evfilter.dll"
bool EventReloadW(string url);
bool EventReloadA(string url);
bool EventInEffectW(datetime dt, string currency, int impact, int minBefore, int minAfter);
bool EventInEffectA(datetime dt, string currency, int impact, int minBefore, int minAfter);
#import