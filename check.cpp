#include <iostream>
#include <fstream>
using namespace std;
int main()
{
    ifstream fin("./log.txt");

    string s;

    long long early = 0, late = 0, bad = 0;

    while (getline(fin, s))
    {
        if (s[0] == 'b')
            ++bad;
        else if (s[0] == 'e')
            ++early;
        else
            ++late;
    }

    cout << "提前帧：" << early << " 错后帧：" << late << " 无法同步帧" << bad << '\n';

    long double x = early * 1.0 / late;

    cout << "比率:" << x << '\n';
}