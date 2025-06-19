#include "AggregateNotesFilter.h"
#include <algorithm>
#include <iterator>

namespace MikuMikuWorld
{
    FlickableNotesFilter CommonNoteFilters::cmnflickFilter{};
    HoldStepNotesFilter CommonNoteFilters::cmnStepFilter{};
    FrictionableNotesFilter CommonNoteFilters::cmnFrictionFilter{};
    GuideNotesFilter CommonNoteFilters::cmnGuideFilter{};
    EaseNotesFilter CommonNoteFilters::cmnEaseFilter{};
    
    AggregateNotesFilter& AggregateNotesFilter::add(NotesFilter* const filter)
    {
        filters.push(filter);
        return *this;
    }

    NoteSelection AggregateNotesFilter::filter(NoteSelection selection, const Score& score)
    {
        NoteSelection existingNotes;
        std::copy_if(selection.begin(), selection.end(), std::inserter(existingNotes, existingNotes.begin()),
            [&](const int id) { return score.notes.find(id) != score.notes.end(); });
        
        while (!filters.empty())
        {
            existingNotes = filters.front()->filter(existingNotes, score);
            filters.pop();
        }
        
        return existingNotes;
    }

    void AggregateNotesFilter::clear()
    {
        filters = std::queue<NotesFilter*>();
    }
}
