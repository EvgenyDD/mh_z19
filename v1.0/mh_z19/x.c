
#include <iostream>

using namespace std;
#define BITS_PER_SIG 17

uint8_t tx_arr[256] = {0};

int main()
{
    for(uint32_t i = BITS_PER_SIG * (8 * 0); i < BITS_PER_SIG * (8 * 1); i++) // g
    {
        uint32_t pos = i % BITS_PER_SIG;
        uint32_t color_bit_off = (/*color->g*/ 0xC0 & (1 << (7 - i / BITS_PER_SIG))) ? (BITS_PER_SIG * 2 / 3) : (BITS_PER_SIG / 3);
        if(pos < color_bit_off)
        {
            tx_arr[i / 8] |= (1 << (7 - (i % 8)));
            cout << "+ ";
        }
        else
        {
            tx_arr[i / 8] &= ~(1 << (7 - (i % 8)));
            cout << "- ";
        }
    }
    cout << endl;

    for(uint32_t i = 0; i < sizeof(tx_arr); i++)
        cout << (int)tx_arr[i] << " ";
    cout << endl;
    return 0;
}
