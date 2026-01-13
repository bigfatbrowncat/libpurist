    #pragma once

    #include "litera_key.h"

    #include <stdexcept>
    #include <vector>
    
    struct row_key : private std::vector<litera_key> {
        int width, height;
        bool operator == (const row_key& other) const {
            return this->width == other.width &&
                   this->height == other.height &&
                   std::equal(this->begin(), this->end(), other.begin(), other.end());
        }

        bool operator < (const row_key& other) const {
            if (this->size() != other.size()) {
                throw std::logic_error(std::string("Incomparable keys - different length: ") + std::to_string(this->size()) + " and " +  std::to_string(other.size()));
            }

            if (this->width < other.width) return true;
            if (this->width > other.width) return false;

            if (this->height < other.height) return true;
            if (this->height > other.height) return false;

            for (size_t i = 0; i < size(); i++) {
                if ((*this)[i] < other[i]) return true;
                if (!((*this)[i] == other[i])) return false;
            }
            return false;
        }

        litera_key& operator [] (size_t i) {
            return std::vector<litera_key>::operator [] (i);
        }

        const litera_key& operator [] (size_t i) const {
            return std::vector<litera_key>::operator [] (i);
        }

        void push_back(const litera_key& lk) {
            std::vector<litera_key>::push_back(lk);
        }
    };
