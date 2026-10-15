#pragma once

#include <string>

using namespace std;

namespace space {
    class SPFontStyle {
    public:
        int height;
        int weight;
        bool italic;
        bool underline;
        string family;
        SPFontStyle(string family, int height, int weight, bool italic, bool underline) {
            this->family = family;
            this->height = height;
            this->weight = weight;
            this->italic = italic;
            this->underline = underline;
        }
        int getHeight() {
            return this->height;
        }
        int getWeight() {
            return this->weight;
        }
        bool isItalic() {
            return this->italic;
        }
        bool isUnderline() {
            return this->underline;
        }
        string getFamily() {
            return this->family;
        }

    };
}

