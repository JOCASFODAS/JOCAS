package br.jocas.modlist;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.time.Instant;
import java.util.*;

/** Plain data shared by the loader and the separate viewer; no Minecraft classes. */
public record Snapshot(String loader, String side, String javaVersion, String os,
                       String capturedAt, List<Entry> mods) {
    public record Entry(String name, String id, String version, String file, String dependencies) {}

    public Snapshot {
        mods = mods.stream().sorted(Comparator.comparing(Entry::name, String.CASE_INSENSITIVE_ORDER)
            .thenComparing(Entry::id)).toList();
    }

    public static Snapshot capture(String loader, String side, List<Entry> mods) {
        return new Snapshot(loader, side, System.getProperty("java.version"),
            System.getProperty("os.name") + " " + System.getProperty("os.arch"), Instant.now().toString(), mods);
    }

    public String version(String id) {
        return mods.stream().filter(m -> m.id().equals(id)).map(Entry::version).findFirst().orElse("não informado");
    }

    public static String clean(String text) {
        return Objects.toString(text, "não informado").replaceAll("[\\p{Cntrl}\\p{Cf}]", " ").strip();
    }

    public String report() {
        StringBuilder out = new StringBuilder("JOCAS LISTA DE MODS — relatório 1.0.0\n");
        out.append("Minecraft: ").append(clean(version("minecraft"))).append('\n');
        out.append("Carregador: ").append(clean(loader)).append(" | Ambiente: ").append(clean(side)).append('\n');
        out.append("Java: ").append(clean(javaVersion)).append(" | Sistema: ").append(clean(os)).append('\n');
        out.append("Coleta (UTC): ").append(clean(capturedAt)).append('\n');
        out.append("Entradas reconhecidas pelo carregador: ").append(mods.size()).append(" (inclui bibliotecas e componentes internos)\n\n");
        out.append("NOME | ID | VERSÃO | ARQUIVO\n");
        for (Entry mod : mods) {
            out.append(clean(mod.name())).append(" | ").append(clean(mod.id())).append(" | ")
                .append(clean(mod.version())).append(" | ").append(clean(mod.file())).append('\n');
            if (!mod.dependencies().isBlank()) out.append("  Dependências declaradas: ").append(clean(mod.dependencies())).append('\n');
        }
        out.append("\nPEDIDO DE ANÁLISE\nAnalise esta lista de mods, suas versões e dependências declaradas. ")
            .append("Aponte possíveis incompatibilidades, diferenciando evidências de hipóteses. ")
            .append("Solicite o latest.log ou crash-report e a descrição do problema quando forem necessários. ")
            .append("Não conclua que um mod causou o erro apenas por aparecer na lista.\n\n")
            .append("Observação: esta lista foi coletada na inicialização; não confirma que todos os mods terminaram de carregar. ")
            .append("Não contém logs, configurações, tokens, IPs ou caminhos completos. ")
            .append("Nada foi enviado automaticamente. Mods apenas no servidor remoto não aparecem aqui.\n");
        return out.toString();
    }

    public void save(Path target) throws IOException {
        Properties p = new Properties();
        p.setProperty("format", "1");
        p.setProperty("loader", loader); p.setProperty("side", side);
        p.setProperty("java", javaVersion); p.setProperty("os", os); p.setProperty("time", capturedAt);
        p.setProperty("count", Integer.toString(mods.size()));
        for (int i = 0; i < mods.size(); i++) {
            Entry m = mods.get(i); String k = "mod." + i + ".";
            p.setProperty(k + "name", m.name()); p.setProperty(k + "id", m.id());
            p.setProperty(k + "version", m.version()); p.setProperty(k + "file", m.file());
            p.setProperty(k + "dependencies", m.dependencies());
        }
        StringWriter writer = new StringWriter(); p.store(writer, "JOCAS Mod List");
        atomicWrite(target, writer.toString().getBytes(StandardCharsets.UTF_8));
    }

    public static Snapshot load(Path path) throws IOException {
        if (Files.size(path) > 32 * 1024 * 1024) throw new IOException("Relatório muito grande.");
        Properties p = new Properties();
        try (Reader r = Files.newBufferedReader(path, StandardCharsets.UTF_8)) { p.load(r); }
        if (!"1".equals(p.getProperty("format"))) throw new IOException("Formato de relatório desconhecido.");
        try {
            int count = Integer.parseInt(p.getProperty("count", "-1"));
            if (count < 0 || count > 100000) throw new IOException("Número de mods inválido.");
            List<Entry> mods = new ArrayList<>();
            for (int i = 0; i < count; i++) {
                String k = "mod." + i + ".";
                mods.add(new Entry(required(p, k + "name"), required(p, k + "id"), required(p, k + "version"),
                    required(p, k + "file"), required(p, k + "dependencies")));
            }
            return new Snapshot(required(p, "loader"), required(p, "side"), required(p, "java"),
                required(p, "os"), required(p, "time"), mods);
        } catch (IllegalArgumentException e) { throw new IOException("Relatório inválido.", e); }
    }

    private static String required(Properties p, String key) throws IOException {
        String value = p.getProperty(key);
        if (value == null) throw new IOException("Campo ausente: " + key);
        return value;
    }

    public static void atomicWrite(Path path, byte[] data) throws IOException {
        Path parent = path.toAbsolutePath().getParent(); Files.createDirectories(parent);
        Path tmp = Files.createTempFile(parent, ".jocas-", ".tmp");
        try {
            Files.write(tmp, data);
            try { Files.move(tmp, path, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING); }
            catch (AtomicMoveNotSupportedException e) { Files.move(tmp, path, StandardCopyOption.REPLACE_EXISTING); }
        } finally { Files.deleteIfExists(tmp); }
    }
}
