

struct keycode {
    const char *c;
    int v;
} keycode;

struct keycode keycodes[] = {

    { "0", 7 }, 
    { "1", 8 }, 
    { "2", 9 }, 
    { "3", 10 }, 
    { "4", 11 }, 
    { "5", 12 }, 
    { "6", 13 }, 
    { "7", 14 }, 
    { "8", 15 }, 
    { "9", 16 }, 
    { "a", 29 }, 
    { "b", 30 }, 
    { "c", 31 }, 
    { "d", 32 }, 
    { "e", 33 }, 
    { "f", 34 }, 
    { "g", 35 }, 
    { "h", 36 }, 
    { "i", 37 }, 
    { "j", 38 }, 
    { "k", 39 }, 
    { "l", 40 }, 
    { "m", 41 }, 
    { "n", 42 }, 
    { "o", 43 }, 
    { "p", 44 }, 
    { "q", 45 }, 
    { "r", 46 }, 
    { "s", 47 }, 
    { "t", 48 }, 
    { "u", 49 }, 
    { "v", 50 }, 
    { "w", 51 }, 
    { "x", 52 }, 
    { "y", 53 }, 
    { "z", 54 }, 
    { "*", 17 },
    { "#", 18 },
    { "KEY_UP", 19},  
    { "KEY_DOWN", 20 }, 
    { "KEY_LEFT", 21 },
    { "KEY_RIGHT", 22 },
    { "," , 55 }, 
    { ".", 56 },
    { "^I", 61 },
    { " ", 62 },
    { "^J", 66 },
    { "KEY_BACKSPACE", 67 },
    { "`", 68 },
    { "-", 69 },
    { "=", 70 },
    { "[", 71 },
    { "]", 72 },
    { "\\", 73 },
    { ";", 74 },
    { "/", 76 },
    { "@", 77 },
    { "+", 81 },
    { "^[",111 },
    { "KEY_DC", 112 },
    //CAPS_LOCK = 115
    { "KEY_F(1)", 131 },
    { "KEY_F(2)", 132 },
    { "KEY_F(3)", 133 },
    { "KEY_F(4)", 134 },
    { "KEY_F(5)", 135 },
    { "KEY_F(6)", 136 },
    { "KEY_F(7)", 137 },
    { "KEY_F(8)", 138 },
    { "KEY_F(9)", 139 },
    { "KEY_F(10)", 140 },
    { "KEY_F(11)", 141 },
    { "KEY_F(12)", 142 },
}; 


int find_keycode(const char* input)
{
    int i;
    for(i=0;i<sizeof(keycodes)/sizeof(keycode);i++)
    {
        if (!strcmp(keycodes[i].c, input)) {
            return keycodes[i].v;
        }
    }
    return -1;
}

int find_keycode_ch(const char ch)
{
    int i;
    for(i=0;i<sizeof(keycodes)/sizeof(keycode);i++)
    {
        if (*keycodes[i].c == ch) {
            return keycodes[i].v;
        }
    }
    return -1;
}

