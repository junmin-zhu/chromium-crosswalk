// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/ninja_copy_target_writer.h"

#include "base/strings/string_util.h"
#include "tools/gn/string_utils.h"
#include "tools/gn/substitution_list.h"
#include "tools/gn/substitution_writer.h"

NinjaCopyTargetWriter::NinjaCopyTargetWriter(const Target* target,
                                             const Toolchain* toolchain,
                                             std::ostream& out)
    : NinjaTargetWriter(target, toolchain, out) {
}

NinjaCopyTargetWriter::~NinjaCopyTargetWriter() {
}

void NinjaCopyTargetWriter::Run() {
  CHECK(target_->action_values().outputs().list().size() == 1);
  const SubstitutionList& output_subst_list =
      target_->action_values().outputs();
  CHECK_EQ(1u, output_subst_list.list().size())
      << "Should have one entry exactly.";
  const SubstitutionPattern& output_subst = output_subst_list.list()[0];

  std::vector<OutputFile> output_files;

  std::string rule_prefix = helper_.GetRulePrefix(target_->settings());

  // Note that we don't write implicit deps for copy steps. "copy" only
  // depends on the output files themselves, rather than having includes
  // (the possibility of generated #includes is the main reason for implicit
  // dependencies).
  //
  // It would seem that specifying implicit dependencies on the deps of the
  // copy command would still be harmeless. But Chrome implements copy tools
  // as hard links (much faster) which don't change the timestamp. If the
  // ninja rule looks like this:
  //   output: copy input | foo.stamp
  // The copy will not make a new timestamp on the output file, but the
  // foo.stamp file generated from a previous step will have a new timestamp.
  // The copy rule will therefore look out-of-date to Ninja and the rule will
  // get rebuilt.
  //
  // If this copy is copying a generated file, not listing the implicit
  // dependency will be fine as long as the input to the copy is properly
  // listed as the output from the step that generated it.
  //
  // Moreover, doing this assumes that the copy step is always a simple
  // locally run command, so there is no need for a toolchain dependency.
  for (size_t i = 0; i < target_->sources().size(); i++) {
    const SourceFile& input_file = target_->sources()[i];

    OutputFile output_file =
        SubstitutionWriter::ApplyPatternToSourceAsOutputFile(
            target_->settings(), output_subst, input_file);
    output_files.push_back(output_file);

    out_ << "build ";
    path_output_.WriteFile(out_, output_file);
    out_ << ": " << rule_prefix << "copy ";
    path_output_.WriteFile(out_, input_file);
    out_ << std::endl;
  }

  // Write out the rule for the target to copy all of them.
  out_ << std::endl << "build ";
  path_output_.WriteFile(out_, helper_.GetTargetOutputFile(target_));
  out_ << ": " << rule_prefix << "stamp";
  for (size_t i = 0; i < output_files.size(); i++) {
    out_ << " ";
    path_output_.WriteFile(out_, output_files[i]);
  }
  out_ << std::endl;
}
