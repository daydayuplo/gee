// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include "common/notify.h"
#include "common/khGetopt.h"
#include "fusion/dbroot/raster_dbroot_context.h"

void
usage(const std::string &progn, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] --imagery|--terrain --config <configfile> <outdir>\n"
          "   Generates the required dbroots into <outdir>\n"
          "   Supported options are:\n"
          "      --help | -?:  Display this usage message\n",
          progn.c_str());
  exit(1);
}



int
main(int argc, char **argv)
{
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    bool imagery = false;
    bool terrain = false;
    std::string configfile;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("config", configfile, &khGetopt::FileExists);
    options.opt("imagery", imagery);
    options.opt("terrain", terrain);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);

    // Validate commandline options
    if (argn >= argc) {
      usage(progname, "No <outdir> specified");
    }
    if (!configfile.size()) {
      usage(progname, "No <configfile> specified");
    }
    if (imagery == terrain) {
      usage(progname, "You must specify exactly one of --imagery or --terrain");
    }
    std::string out_dir = GetNextArgAsString();


    // create my context object - holds all the things that only
    // need to be loaded/initialized once
    RasterDbrootContext context(configfile, imagery);

    // generate all the dbroots and icons
    context.EmitAll(out_dir, geProtoDbroot::kProtoFormat);
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}
