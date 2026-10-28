package br.jocas.modlist;

import javax.swing.*;
import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.datatransfer.DataFlavor;
import java.awt.image.BufferedImage;
import java.io.File;
import java.util.List;

/** Run only on an isolated test display: it writes to that display's clipboard. */
public class UiSmoke {
    public static void main(String[] args) throws Exception {
        if (!"yes".equals(System.getenv("JOCAS_ISOLATED_DISPLAY"))) throw new IllegalStateException("Use an isolated display");
        SwingUtilities.invokeAndWait(() -> {
            Viewer viewer = null;
            try {
                Viewer.configureUi();
                var data = new Snapshot("NeoForge 26.1.2.109", "cliente", "25.0.1", "Linux amd64", "2026-10-22T18:00:00Z", List.of(
                    new Snapshot.Entry("Minecraft", "minecraft", "26.1.2", "componente interno", ""),
                    new Snapshot.Entry("JOCAS Lista de Mods", "jocas_modlist", "1.0.0", "JOCAS-Lista-de-Mods-1.0.0-neoforge.jar", "minecraft [1.20.2,) (REQUIRED, BOTH)"),
                    new Snapshot.Entry("Exemplo de mod (dados de teste)", "exemplo", "2.3.0", "exemplo.jar", "minecraft >=1.20.1")));
                viewer = new Viewer(data); viewer.frame.setVisible(true);
                viewer.search.setText("jocas");
                if (viewer.table.getRowCount() != 1) throw new AssertionError("search failed");
                viewer.copy.doClick();
                Object clipboard = Toolkit.getDefaultToolkit().getSystemClipboard().getData(DataFlavor.stringFlavor);
                if (!data.report().equals(clipboard)) throw new AssertionError("copy did not include full report with filter active");
                viewer.search.setText("[");
                if (viewer.table.getRowCount() != 0) throw new AssertionError("search should be literal");
                viewer.search.setText("");
                BufferedImage image = new BufferedImage(viewer.frame.getWidth(), viewer.frame.getHeight(), BufferedImage.TYPE_INT_RGB);
                Graphics2D g = image.createGraphics(); viewer.frame.paintAll(g); g.dispose();
                ImageIO.write(image, "png", new File(args[0]));
                System.out.println("OK: busca literal, botão Copiar, clipboard completo com filtro ativo e renderização da janela.");
            } catch (Exception e) { throw new RuntimeException(e); }
            finally { if (viewer != null) viewer.frame.dispose(); }
        });
    }
}
