#!/usr/bin/env ruby

require_relative "modules/assembler"
require_relative "modules/encoder"
require "tempfile"

ELF_WRITER_PATH = File.expand_path("../build/toy_elf_writer", __dir__)

def output_path_for(source_path)
  ext = File.extname(source_path)
  "#{source_path.delete_suffix(ext)}.elf"
end

def write_elf(output_path, text, data)
  unless File.executable?(ELF_WRITER_PATH)
    raise "ELF writer is missing; build the toy_elf_writer CMake target"
  end

  Tempfile.create("toy_text") do |text_file|
    Tempfile.create("toy_data") do |data_file|
      text_file.binmode
      data_file.binmode
      text_file.write(text)
      data_file.write(data)
      text_file.close
      data_file.close

      success = system(
        ELF_WRITER_PATH,
        output_path,
        text_file.path,
        data_file.path
      )
      raise "ELF writer failed" unless success
    end
  end
end

def main(arguments)
  unless (1..2).cover?(arguments.length)
    warn "Usage: #{File.basename($PROGRAM_NAME)} <source.rb> [output.elf]"
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

  write_elf(output_path, words.pack("V*"), "".b)
  0
end

exit(main(ARGV)) if __FILE__ == $PROGRAM_NAME
