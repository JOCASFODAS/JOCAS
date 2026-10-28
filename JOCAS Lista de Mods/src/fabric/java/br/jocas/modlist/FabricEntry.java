package br.jocas.modlist;

import net.fabricmc.api.ModInitializer;
import net.fabricmc.api.EnvType;
import net.fabricmc.loader.api.FabricLoader;
import net.fabricmc.loader.api.ModContainer;
import net.fabricmc.loader.api.metadata.ModOrigin;
import java.util.*;
import java.util.stream.Collectors;

public final class FabricEntry implements ModInitializer {
    @Override public void onInitialize() {
        try {
            FabricLoader loader = FabricLoader.getInstance();
            boolean client = loader.getEnvironmentType() == EnvType.CLIENT;
            String version = loader.getModContainer("fabricloader").map(c -> c.getMetadata().getVersion().getFriendlyString()).orElse("?");
            Export.start(Snapshot.capture("Fabric " + version, client ? "cliente" : "servidor dedicado",
                collect(loader.getAllMods())), loader.getGameDir(), client);
        } catch (Exception | LinkageError e) { Export.failure(e); }
    }

    public static List<Snapshot.Entry> collect(Collection<ModContainer> containers) {
        List<Snapshot.Entry> mods = new ArrayList<>();
        for (ModContainer container : containers) {
            var m = container.getMetadata();
            String file = "componente interno";
            var origin = container.getOrigin();
            if (origin.getKind() == ModOrigin.Kind.PATH) {
                file = origin.getPaths().stream().map(p -> p.getFileName().toString()).collect(Collectors.joining(", "));
            } else if (origin.getKind() == ModOrigin.Kind.NESTED) {
                file = FmlAdapter.basename(origin.getParentSubLocation()) + " (incluído em " + origin.getParentModId() + ")";
            }
            String deps = m.getDependencies().stream().map(d -> d.getModId() + " " +
                d.getVersionRequirements().stream().map(Object::toString).sorted().collect(Collectors.joining(" OU ")) +
                " (" + d.getKind() + ")").sorted().collect(Collectors.joining("; "));
            mods.add(new Snapshot.Entry(m.getName(), m.getId(), m.getVersion().getFriendlyString(), file, deps));
        }
        return mods;
    }
}
