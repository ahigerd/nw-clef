#include "codec/sampledata.h"

// This include should come last to avoid namespace collisions.
#include "plugin/baseplugin.h"

#ifdef BUILD_CLAP
#include "plugin/clapplugin.h"
#endif

// In the functions below, ctx->openFile() is provided by the plugin interface. Use
// this instead of standard library functions to open additional files in order to use
// the host's virtual filesystem.

struct ClefPluginInfo {
  CLEF_PLUGIN_STATIC_FIELDS
#ifdef BUILD_CLAP
  using ClapPlugin = ClefClapPlugin<ClefPluginInfo>;
#endif

  static bool isPlayable(std::istream& file) {
    // Implementations should check to see if the file is supported.
    // Return false or throw an exception to report failure.
    return false;
  }

  static int sampleRate(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the sample rate of the file.
    // This can be hard-coded if the plugin always uses the same sample rate.
    return 48000;
  }

  static double length(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the length of the file in seconds.
    return 0;
  }

  static TagMap readTags(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should read the tags from the file.
    // If the file format does not support embedded tags, consider
    // inheriting from TagsM3UMixin and removing this function.
    return TagMap();
  }

  SynthContext* prepare(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Prepare to play the file. Load any necessary data into memory and store any
    // applicable state in members on this plugin object.

    // Be sure to call this to clear the sample cache:
    ctx->purgeSamples();

    return nullptr;
  }

  void release() {
    // Release any retained state allocated in prepare().
  }
};

const std::string ClefPluginInfo::version = "0.0.1";
const std::string ClefPluginInfo::pluginName = "nw-clef Plugin";
const std::string ClefPluginInfo::pluginShortName = "nw-clef";
ConstPairList ClefPluginInfo::extensions = { { "brsar", "BRSAR files (*.brsar)" } };
const std::string ClefPluginInfo::about =
  "nw-clef Plugin copyright (C) 2024 Adam Higerd\n"
  "Distributed under the MIT license.";

CLEF_PLUGIN(ClefPluginInfo);
