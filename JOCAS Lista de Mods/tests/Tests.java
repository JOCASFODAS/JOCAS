package br.jocas.modlist;

import java.nio.file.*;
import java.util.*;
import java.lang.reflect.Proxy;
import net.fabricmc.loader.api.ModContainer;
import net.fabricmc.loader.api.Version;
import net.fabricmc.loader.api.metadata.*;

public class Tests {
    static int checks;
    static void check(boolean condition, String name) {
        if (!condition) throw new AssertionError(name);
        checks++;
    }
    public interface OldDependency {
        String getModId(); String getVersionRange(); boolean isMandatory(); String getSide();
    }
    public interface NewDependency {
        String getModId(); String getVersionRange(); String getType(); String getSide();
    }
    public interface ModInfo {
        String getDisplayName(); String getModId(); String getVersion(); Object getOwningFile(); List<?> getDependencies();
    }
    static <T> T proxy(Class<T> type, Map<String, Object> values) {
        return type.cast(Proxy.newProxyInstance(type.getClassLoader(), new Class[]{type},
            (p, method, args) -> {
                if (values.containsKey(method.getName())) return values.get(method.getName());
                throw new UnsupportedOperationException(method.toString());
            }));
    }

    public static void main(String[] args) throws Exception {
        Path temp = Files.createTempDirectory("jocas-tests-");
        try {
            List<Snapshot.Entry> entries = new ArrayList<>();
            entries.add(new Snapshot.Entry("Minecraft", "minecraft", "26.1.2", "componente interno", ""));
            for (int i = 0; i < 700; i++) entries.add(new Snapshot.Entry("Mod áç " + i, "mod_" + i, "1.0", "mod-" + i + ".jar", "minecraft >=1.20.1"));
            Snapshot data = new Snapshot("NeoForge 26.1.2.109", "cliente", "25", "Linux amd64", "2026-10-22T00:00:00Z", entries);
            Export.write(data, temp);
            Snapshot loaded = Snapshot.load(temp.resolve("snapshot.properties"));
            check(loaded.equals(data), "UTF-8 and 701 entries round-trip");
            String report = Files.readString(temp.resolve("relatorio-mods.txt"));
            check(report.equals(data.report()), "saved report matches clipboard source");
            check(report.contains("Minecraft: 26.1.2"), "game version");
            check(report.contains("mod_699") && report.contains("mod_0"), "no truncation on large packs");
            check(report.contains("minecraft >=1.20.1"), "dependencies in report");
            check(!report.contains(temp.toString()), "no absolute path in report");
            check(Snapshot.clean("teste\n\t\u202eX").equals("teste   X"), "control characters sanitized");
            check(FmlAdapter.basename("C:\\Users\\Someone\\mods\\teste.jar").equals("teste.jar"), "Windows path redacted");
            check(FmlAdapter.basename("/home/someone/mods/teste.jar").equals("teste.jar"), "Linux path redacted");
            Files.writeString(temp.resolve("broken.properties"), "format=1\ncount=-1");
            try { Snapshot.load(temp.resolve("broken.properties")); throw new AssertionError("bad count accepted"); }
            catch (java.io.IOException expected) { checks++; }
            Files.writeString(temp.resolve("broken.properties"), "format=2\ncount=0");
            try { Snapshot.load(temp.resolve("broken.properties")); throw new AssertionError("unknown format accepted"); }
            catch (java.io.IOException expected) { checks++; }

            Object old = proxy(OldDependency.class, Map.of("getModId", "minecraft", "getVersionRange", "[1.20.1,)", "isMandatory", true, "getSide", "BOTH"));
            Object neo = proxy(NewDependency.class, Map.of("getModId", "fabric", "getVersionRange", "*", "getType", "INCOMPATIBLE", "getSide", "CLIENT"));
            Object mod = proxy(ModInfo.class, Map.of("getDisplayName", "Teste", "getModId", "teste", "getVersion", "1.2",
                "getDependencies", List.of(old, neo), "getOwningFile", new Object()));
            var collected = FmlAdapter.collect(List.of(mod));
            check(collected.get(0).dependencies().contains("REQUIRED"), "old Forge mandatory API");
            check(collected.get(0).dependencies().contains("INCOMPATIBLE"), "new NeoForge dependency API");
            check(collected.get(0).file().equals("arquivo não informado"), "missing filename does not lose mod");

            ModMetadata metadata = proxy(ModMetadata.class, Map.of("getName", "Acentuação", "getId", "teste", "getVersion", Version.parse("1.2.3"), "getDependencies", List.of()));
            ModOrigin origin = proxy(ModOrigin.class, Map.of("getKind", ModOrigin.Kind.NESTED,
                "getParentSubLocation", "META-INF/jars/nested.jar", "getParentModId", "pai"));
            ModContainer container = proxy(ModContainer.class, Map.of("getMetadata", metadata, "getOrigin", origin));
            var fabric = FabricEntry.collect(List.of(container));
            check(fabric.get(0).file().equals("nested.jar (incluído em pai)"), "nested Fabric mod tracked");
            check(fabric.get(0).version().equals("1.2.3"), "real Fabric Version API");
            check(fabric.get(0).name().equals("Acentuação"), "Fabric unicode metadata");
            System.out.println("OK: " + checks + " verificações de relatório, listas grandes, privacidade e adaptadores.");
        } finally {
            try (var paths = Files.walk(temp)) {
                for (Path path : paths.sorted(Comparator.reverseOrder()).toList()) Files.delete(path);
            }
        }
    }
}
