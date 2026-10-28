package br.jocas.modlist;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.concurrent.atomic.AtomicBoolean;

/** The viewer runs outside Minecraft, whose JVM can force AWT headless mode. */
public final class Export {
    private static final AtomicBoolean STARTED = new AtomicBoolean();
    private Export() {}

    public static void start(Snapshot snapshot, Path gameDir, boolean client) {
        if (!STARTED.compareAndSet(false, true)) return;
        Thread worker = new Thread(() -> {
            try {
                Path dir = gameDir.toAbsolutePath().resolve("jocas-modlist");
                write(snapshot, dir);
                System.out.println("[JOCAS] Lista de mods salva em jocas-modlist/relatorio-mods.txt");
                if (client && !Boolean.getBoolean("jocas.modlist.noWindow")) {
                    Path viewer = dir.resolve("JOCAS-Lista-de-Mods.jar");
                    try (InputStream in = Export.class.getResourceAsStream("/jocas-viewer.jar")) {
                        if (in == null) throw new IOException("Visualizador não encontrado no mod.");
                        Snapshot.atomicWrite(viewer, in.readAllBytes());
                    }
                    new ProcessBuilder(javaExecutable(), "-Xmx128m", "-Djava.awt.headless=false", "-jar",
                        viewer.toString(), dir.resolve("snapshot.properties").toString(),
                        "--parent", Long.toString(ProcessHandle.current().pid()))
                        .redirectErrorStream(true).redirectOutput(dir.resolve("janela.log").toFile()).start();
                }
            } catch (Exception | LinkageError e) { failure(e); }
        }, "JOCAS-exportar-mods");
        worker.setDaemon(true); worker.start();
    }

    public static void write(Snapshot snapshot, Path dir) throws IOException {
        snapshot.save(dir.resolve("snapshot.properties"));
        Snapshot.atomicWrite(dir.resolve("relatorio-mods.txt"), snapshot.report().getBytes(StandardCharsets.UTF_8));
    }

    static String javaExecutable() {
        String exe = System.getProperty("os.name").startsWith("Windows") ? "javaw.exe" : "java";
        return Path.of(System.getProperty("java.home"), "bin", exe).toString();
    }

    public static void failure(Throwable e) {
        System.err.println("[JOCAS] Não foi possível exportar/abrir a lista de mods: " + e);
    }
}
