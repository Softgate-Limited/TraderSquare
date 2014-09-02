#import "Economic.dll"
bool     EconomicReload(string url);
int      EconomicGetIndex(int offset);
datetime EconomicGetDateTime(int index);
int      EconomicGetImpact(int index);
string   EconomicGetCurrency(int index);
string   EconomicGetTitle(int index);
#import