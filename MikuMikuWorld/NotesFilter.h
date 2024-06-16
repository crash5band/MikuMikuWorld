#pragma once
#include <functional>
#include <unordered_set>

#include "Score.h"
typedef std::unordered_set<int> NoteSelection;
namespace MikuMikuWorld
{
    class NotesFilter
    {
    public:
        virtual ~NotesFilter() {}
        virtual NoteSelection filter(NoteSelection selection, const Score& score) const = 0;
    };

    class FlickableNotesFilter final : public NotesFilter
    {
    public:
        bool canFlick(int noteId, const Score& score) const;
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
    };

    class HoldStepNotesFilter : public NotesFilter
    {
    public:
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
    };

    class FrictionableNotesFilter : public NotesFilter
    {
    public:
        bool canToggleFriction(int noteId, const Score& score) const;
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
    };

    class InverseNotesFilter : public NotesFilter
    {
    public:
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
        explicit InverseNotesFilter(NotesFilter* filter) : originalFilter{ filter } {}
    private:
        NotesFilter* originalFilter;
    };

    class GuideNotesFilter : public NotesFilter
    {
    public:
        bool isGuideHold(int noteId, const Score& score) const;
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
    };

    class EaseNotesFilter : public NotesFilter
    {
    public:
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
    };

    class CustomFilter : public NotesFilter
    {
    public:
        NoteSelection filter(NoteSelection selection, const Score& score) const override;
        CustomFilter(std::function<bool(int)> pred) : predicate{ pred } {}
    private:
        std::function<bool(int)> predicate;
    };
}