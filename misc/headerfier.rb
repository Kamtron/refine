#!/usr/bin/env ruby

ARGV.each do |file|
  lines = IO.readlines(file)
  first_include = 0
  lines.each_with_index do |line,i|
    first_include = i
    break if ( line =~ /#include/ )
    puts line
  end
  File.open(file,"w") do |f|
    f.puts <<EOF

/* Copyright 2007 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

EOF
    (first_include..(lines.size-1)).each do |i|
      f.puts lines[i]
    end
  end
end
