#!/usr/bin/env ruby

require_relative "modules/assembler"
require_relative "modules/encoder"

def output_path_for(source_path)
  return "#{source_path.delete_suffix(".rb")}.bin" if source_path.end_with?(".rb")

  "#{source_path}.bin"
end

def main(arguments)
  unless (1..2).cover?(arguments.length)
    warn "Usage: #{File.basename($PROGRAM_NAME)} <source.rb> [output.bin]"
    return 1
  end

  source_path = arguments[0]
  output_path = arguments[1] || output_path_for(source_path)

  assembler = Assembler.new
  assembler.instance_eval(File.read(source_path), source_path)

  encoder = Encoder.new(assembler.labels)
  words = assembler.instructions.map do |instruction|
    encoder.encode(instruction)
  end

  File.binwrite(output_path, words.pack("V*"))
  0
end

exit(main(ARGV)) if __FILE__ == $PROGRAM_NAME
