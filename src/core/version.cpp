#include "core/version.h"

#include "version_generated.h"  // configure_file'd from cmake/version.h.in

#include <string>

namespace altair {

const char* versionNumber() { return PASCAL100_VERSION; }
const char* versionCommit() { return PASCAL100_COMMIT; }

bool versionDirty() { return PASCAL100_DIRTY != 0; }

const char* versionString() {
    // Built once and kept, because callers hand it straight to a stream and the
    // three of them together would otherwise rebuild the same string three times.
    static const std::string s = [] {
        const std::string commit = versionCommit();

        // ONE VERSION, NOT TWO. This line used to read
        //
        //     pascal100 0.1.0 (v0.1.0-51-g1e58347)
        //
        // which says 0.1.0 twice: `git describe` ALREADY BEGINS WITH THE TAG, so the
        // number in front of the parenthesis is the same number again, and the reader
        // has to notice that before they can tell which part is the new information.
        // The describe string on its own carries all of it -- the release it is built
        // on, how far past it, and which commit.
        //
        // So the normal line is the describe, with its leading `v` dropped because
        // this is prose and not a tag:
        //
        //     pascal100 0.1.0-51-g1e58347
        //     pascal100 0.1.0                    (built exactly on the tag)
        // pascal100's tags are `pascal100-v0.1.0` (the repo also holds altairsim's v1.3.x
        // tags; CMake's describe --match skips them), so the prefix dropped is that.
        std::string v = "pascal100 ";
        const std::string prefix = "pascal100-v";
        const std::string tag = prefix + std::string(PASCAL100_VERSION);
        if (commit.rfind(tag, 0) == 0) v += commit.substr(prefix.size());

        // ...AND THE CASES WHERE IT DOES NOT. A history with no tags describes to a
        // bare sha, which names the commit but not the release; a source tarball has no
        // git in it at all. Both have to say the version themselves, so the parenthesis
        // comes back -- and it is now carrying something the line does not already say.
        //
        // The mismatch case lands here too, deliberately: if the nearest tag is not this
        // build's version the two really do differ, and printing both is the point.
        //
        // "commit unknown" rather than a bare "unknown", so it still reads as a sentence
        // when there is nothing to put in it.
        else if (commit == "unknown") v += std::string(PASCAL100_VERSION) + " (commit unknown)";
        else v += std::string(PASCAL100_VERSION) + " (" + commit + ")";

        // MODIFIED, not "dirty": the operator reading this line wants to know the binary
        // does not match the commit named next to it, and "dirty" is a word that only
        // means that to someone who already knew. It is its own parenthesis now rather
        // than a clause inside the commit's, because in the normal case above there is
        // no longer a parenthesis for it to sit in.
        if (versionDirty()) v += " (modified)";
        return v;
    }();
    return s.c_str();
}

}  // namespace altair
