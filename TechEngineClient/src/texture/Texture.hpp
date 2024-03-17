#pragma once

#include <string>

namespace TechEngine {
    class Texture {
    private:
        std::string m_filepath;
        std::string m_name;
        unsigned int m_id;
        int m_width, m_height, m_channels;

    public:
        Texture(const std::string &filepath);

        ~Texture();

        void bind(unsigned int slot = 0) const;

        void unbind() const;

        inline int getWidth() const {
            return m_width;
        }

        inline int getHeight() const {
            return m_height;
        }

        inline int getChannels() const {
            return m_channels;
        }

        inline unsigned int getID() const {
            return m_id;
        }

        inline const std::string &getFilepath() const {
            return m_filepath;
        }

        inline const std::string &getName() const {
            return m_name;
        }
    };
}