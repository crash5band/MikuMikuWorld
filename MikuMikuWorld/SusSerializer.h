#pragma once
#include "ScoreSerializer.h"

namespace MikuMikuWorld
{
    class Note;
    class SUSNote;
    class SUS;

    class SusSerializer : public ScoreSerializer
    {
        std::string exportComment;

        std::pair<int, int> barLengthToFraction(float length, float fractionDenom);
        std::string noteKey(const SUSNote& note);
        std::string noteKey(const Note& note);

        Score susToScore(const SUS& sus);
        SUS scoreToSus(const Score& score);

    public:
        SusSerializer();

        void serialize(const Score& score, std::string filename) override;
        Score deserialize(std::string filename) override;
    };
}

