package com.cowlark.fluxengine.buildtools;

import com.cowlark.fluxengine.cli.Command;
import com.cowlark.fluxengine.cli.ReadCommand;
import com.cowlark.fluxengine.cli.WriteCommand;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.core.Logger;
import com.google.common.collect.ImmutableList;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Random;

/**
 * A round-trip encode/decode test for a single format, ported from
 * scripts/encodedecodetest.sh. Generates a random sector image, writes it out
 * as flux with the WriteCommand, reads it back with the ReadCommand, and checks
 * that the two images match.
 *
 * <p>Arguments: {@code format ext [flags...]}, where {@code ext} is the flux
 * file extension ({@code scp} or {@code flux}) and the flags are the extra
 * format-specific options (e.g. {@code --360}). The {@code -c} config flag,
 * {@code --drive.rotational_period_ms=200}, and the file names are supplied by
 * this program.
 */
public class EncodeDecodeTest
{
    public static void main(String[] args) throws Exception
    {
        Logger.setLogger(LogRenderer.create(System.out)::add);

        String format = args[0];
        String ext = args[1];
        ImmutableList<String> flags =
                ImmutableList.copyOf(java.util.Arrays.asList(args).subList(2, args.length));

        Path dir = Files.createTempDirectory("encodedecodetest");
        Path srcFile = dir.resolve("src.img");
        Path fluxFile = dir.resolve("flux." + ext);
        Path destFile = dir.resolve("dest.img");

        writeRandomImage(srcFile);

        run(new WriteCommand(),
                ImmutableList.<String>builder()
                        .add("-c", format, "-i", srcFile.toString(), "-d", fluxFile.toString())
                        .add("--drive.rotational_period_ms=200")
                        .add("--no-verify")
                        .addAll(flags)
                        .build());

        run(new ReadCommand(),
                ImmutableList.<String>builder()
                        .add("-c", format, "-s", fluxFile.toString(), "-o", destFile.toString())
                        .add("--drive.rotational_period_ms=200")
                        .addAll(flags)
                        .build());

        long destSize = Files.size(destFile);
        if (destSize == 0)
        {
            System.err.println("Zero length output file!");
            System.exit(1);
        }

        /* Make the source file the same length as the destination, ported from
         * the script's `truncate -r $destfile $srcfile`. */
        try (RandomAccessFile raf = new RandomAccessFile(srcFile.toFile(), "rw"))
        {
            raf.setLength(destSize);
        }

        long firstDifference = Files.mismatch(srcFile, destFile);
        if (firstDifference != -1)
        {
            System.err.printf("Comparison failed at offset %d!\n", firstDifference);
            System.err.println("Run this to repeat:");
            System.err.println(
                    "bazel run //java/com/cowlark/fluxengine/buildtools:encodedecodetest_bin -- " +
                            String.join(" ", args));
            System.exit(1);
        }
    }

    private static void run(Command command, ImmutableList<String> args) throws Exception
    {
        System.out.printf("fluxengine %s %s%n",
                command instanceof WriteCommand ? "write" : "read",
                String.join(" ", args));
        command.run(args);
    }

    private static void writeRandomImage(Path path) throws IOException
    {
        /* The data is of no value, so a cheap PRNG is fine. */
        Random random = new Random();
        byte[] data = new byte[2 * 1024 * 1024];
        random.nextBytes(data);
        Files.write(path, data);
    }
}
