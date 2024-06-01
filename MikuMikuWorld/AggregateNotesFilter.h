#pragma once
#include <queue>

#include "NotesFilter.h"

namespace MikuMikuWorld
{
    class CommonNoteFilters
    {
    public:
        static FlickableNotesFilter* flickableFilter() { return &cmnflickFilter; }
        static StepNotesFilter* stepFilter() { return &cmnStepFilter; }
        static AdjustableFrictionNotesFilter* frictionableFilter() { return &cmnFrictionFilter; }
        static GuideNotesFilter* guideFilter() { return &cmnGuideFilter; }
        static EaseNotesFilter* easeFilter() { return &cmnEaseFilter; }
    private:
        static FlickableNotesFilter cmnflickFilter;
        static StepNotesFilter cmnStepFilter;
        static AdjustableFrictionNotesFilter cmnFrictionFilter;
        static GuideNotesFilter cmnGuideFilter;
        static EaseNotesFilter cmnEaseFilter;
    };
    
    class AggregateNotesFilter
    {
    public:
        AggregateNotesFilter& add(NotesFilter* filter);
        NoteSelection filter(NoteSelection selection, const Score& score);
        void clear();

    private:
        std::queue<NotesFilter*> filters;
    };
}
