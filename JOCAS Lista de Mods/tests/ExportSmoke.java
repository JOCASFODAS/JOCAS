package br.jocas.modlist;

import java.nio.file.*;
import java.util.List;

/** Verify extraction/launch of the viewer embedded in the distributed mod JAR. */
public class ExportSmoke {
    public static void main(String[] args) throws Exception {
        if (!"yes".equals(System.getenv("JOCAS_ISOLATED_DISPLAY"))) throw new IllegalStateException("Use an isolated display");
        Path dir = Path.of(args[0]);
        Snapshot data = Snapshot.capture("Teste de integração", "cliente", List.of(
            new Snapshot.Entry("Minecraft", "minecraft", "26.1.2", "interno", "")));
        Export.start(data, dir, true);
        long until = System.nanoTime() + 8_000_000_000L;
        ProcessHandle child = null;
        while (System.nanoTime() < until) {
            child = ProcessHandle.current().children().findFirst().orElse(null);
            if (child != null && Files.exists(dir.resolve("jocas-modlist/relatorio-mods.txt"))) break;
            Thread.sleep(100);
        }
        if (child == null) throw new AssertionError("Viewer did not launch");
        Thread.sleep(3000);
        if (!child.isAlive()) throw new AssertionError("Viewer exited unexpectedly");
        if (!Snapshot.load(dir.resolve("jocas-modlist/snapshot.properties")).equals(data)) throw new AssertionError("Snapshot mismatch");
        Files.writeString(dir.resolve("viewer.pid"), Long.toString(child.pid()));
        System.out.println("OK: recurso extraído do JAR final, snapshot salvo e visualizador iniciado em processo separado.");
    }
}
