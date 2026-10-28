package br.jocas.modlist;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.event.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.event.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.Locale;

/** Standalone desktop window. Closing it cannot close Minecraft. */
public final class Viewer {
    final JFrame frame = new JFrame("JOCAS • Lista de Mods");
    final JButton copy = new JButton("Copiar relatório para o GPT");
    final JTextField search = new JTextField();
    final JLabel status = new JLabel("O relatório completo inclui as dependências declaradas.");
    final DefaultTableModel model = new DefaultTableModel(new String[]{"Mod", "ID", "Versão", "Arquivo"}, 0) {
        @Override public boolean isCellEditable(int row, int column) { return false; }
    };
    final JTable table = new JTable(model);
    final TableRowSorter<DefaultTableModel> sorter = new TableRowSorter<>(model);
    final JTextArea preview = new JTextArea();
    final JLabel summary = new JLabel();
    Snapshot snapshot;

    Viewer(Snapshot snapshot) {
        frame.setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
        JPanel root = new JPanel(new BorderLayout(0, 18));
        root.setBorder(new EmptyBorder(24, 26, 20, 26));
        JPanel top = new JPanel(); top.setLayout(new BoxLayout(top, BoxLayout.Y_AXIS));
        JLabel brand = new JLabel("JOCAS  /  LISTA DE MODS");
        brand.setFont(brand.getFont().deriveFont(Font.BOLD, 24f));
        top.add(brand); top.add(Box.createVerticalStrut(8)); top.add(summary);
        top.add(Box.createVerticalStrut(18));
        JLabel searchLabel = new JLabel("Buscar por nome, ID, versão ou arquivo");
        searchLabel.setLabelFor(search); top.add(searchLabel); top.add(Box.createVerticalStrut(6));
        search.setMaximumSize(new Dimension(Integer.MAX_VALUE, 34));
        search.getAccessibleContext().setAccessibleName("Buscar mods"); top.add(search);
        root.add(top, BorderLayout.NORTH);

        table.setRowSorter(sorter); table.setRowHeight(29); table.setFillsViewportHeight(true);
        table.setShowVerticalLines(false); table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        table.getTableHeader().setReorderingAllowed(false);
        table.getColumnModel().getColumn(0).setPreferredWidth(220);
        table.getColumnModel().getColumn(1).setPreferredWidth(170);
        table.getColumnModel().getColumn(2).setPreferredWidth(130);
        table.getColumnModel().getColumn(3).setPreferredWidth(280);
        preview.setEditable(false); preview.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 13));
        preview.setMargin(new Insets(12, 12, 12, 12));
        JTabbedPane tabs = new JTabbedPane();
        tabs.addTab("Lista de mods", new JScrollPane(table));
        tabs.addTab("Relatório completo", new JScrollPane(preview));
        root.add(tabs, BorderLayout.CENTER);

        JPanel bottom = new JPanel(new BorderLayout(0, 10));
        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 0));
        copy.setFont(copy.getFont().deriveFont(Font.BOLD));
        copy.addActionListener(e -> copyReport()); buttons.add(copy);
        JButton save = new JButton("Salvar .txt"); save.addActionListener(e -> saveReport()); buttons.add(save);
        JButton open = new JButton("Abrir outro relatório"); open.addActionListener(e -> openReport()); buttons.add(open);
        bottom.add(buttons, BorderLayout.NORTH);
        JPanel notes = new JPanel(); notes.setLayout(new BoxLayout(notes, BoxLayout.Y_AXIS));
        notes.add(status); notes.add(Box.createVerticalStrut(6));
        notes.add(new JLabel("Copia todos os mods, mesmo com a busca ativa. Nada é enviado automaticamente."));
        bottom.add(notes, BorderLayout.CENTER); root.add(bottom, BorderLayout.SOUTH);
        frame.setContentPane(root); frame.setSize(1040, 680); frame.setMinimumSize(new Dimension(780, 440));
        frame.setLocationRelativeTo(null);
        search.getDocument().addDocumentListener(new DocumentListener() {
            public void insertUpdate(DocumentEvent e) { filter(); }
            public void removeUpdate(DocumentEvent e) { filter(); }
            public void changedUpdate(DocumentEvent e) { filter(); }
        });
        setSnapshot(snapshot);
    }

    void setSnapshot(Snapshot value) {
        snapshot = value; model.setRowCount(0);
        for (var m : value.mods()) model.addRow(new Object[]{Snapshot.clean(m.name()), Snapshot.clean(m.id()),
            Snapshot.clean(m.version()), Snapshot.clean(m.file())});
        summary.setText(value.mods().size() + " entradas  •  Minecraft " + Snapshot.clean(value.version("minecraft")) +
            "  •  " + Snapshot.clean(value.loader()) + "  •  " + Snapshot.clean(value.side()));
        preview.setText(value.report()); preview.setCaretPosition(0);
        status.setText("Coletado em " + value.capturedAt() + " (UTC). Inclui bibliotecas e componentes internos.");
    }

    void filter() {
        String query = search.getText().toLowerCase(Locale.ROOT);
        sorter.setRowFilter(new RowFilter<>() {
            public boolean include(Entry<? extends DefaultTableModel, ? extends Integer> entry) {
                for (int i = 0; i < entry.getValueCount(); i++)
                    if (entry.getStringValue(i).toLowerCase(Locale.ROOT).contains(query)) return true;
                return false;
            }
        });
    }

    void copyReport() {
        try {
            Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(snapshot.report()), null);
            status.setText("Copiado! Cole no chat do GPT com Ctrl+V. Mantenha esta janela aberta até colar.");
        } catch (IllegalStateException | SecurityException e) {
            status.setText("Área de transferência indisponível. Tente novamente ou use Salvar .txt.");
        }
    }

    void saveReport() {
        JFileChooser chooser = new JFileChooser(); chooser.setSelectedFile(new File("JOCAS-relatorio-mods.txt"));
        if (chooser.showSaveDialog(frame) != JFileChooser.APPROVE_OPTION) return;
        Path path = chooser.getSelectedFile().toPath();
        if (Files.exists(path) && JOptionPane.showConfirmDialog(frame, "Substituir o arquivo existente?",
            "Salvar relatório", JOptionPane.YES_NO_OPTION) != JOptionPane.YES_OPTION) return;
        try { Snapshot.atomicWrite(path, snapshot.report().getBytes(StandardCharsets.UTF_8)); status.setText("Relatório salvo."); }
        catch (IOException e) { error(frame, e); }
    }

    void openReport() {
        JFileChooser chooser = new JFileChooser(); chooser.setDialogTitle("Selecione snapshot.properties da pasta jocas-modlist");
        if (chooser.showOpenDialog(frame) != JFileChooser.APPROVE_OPTION) return;
        try { setSnapshot(Snapshot.load(chooser.getSelectedFile().toPath())); }
        catch (IOException e) { error(frame, e); }
    }

    static void error(Component parent, Exception e) {
        JOptionPane.showMessageDialog(parent, e.getMessage(), "JOCAS • Não foi possível abrir/salvar", JOptionPane.ERROR_MESSAGE);
    }

    static void configureUi() throws Exception {
        for (UIManager.LookAndFeelInfo look : UIManager.getInstalledLookAndFeels()) {
            if (look.getName().equals("Nimbus")) { UIManager.setLookAndFeel(look.getClassName()); break; }
        }
        UIManager.put("defaultFont", new Font(Font.SANS_SERIF, Font.PLAIN, 14));
    }

    public static void main(String[] args) {
        if (GraphicsEnvironment.isHeadless()) {
            System.err.println("Sem área de trabalho gráfica. Abra jocas-modlist/relatorio-mods.txt."); return;
        }
        SwingUtilities.invokeLater(() -> {
            try {
                configureUi();
                Path data;
                if (args.length > 0) data = Path.of(args[0]);
                else {
                    Path jarDir = Path.of(Viewer.class.getProtectionDomain().getCodeSource().getLocation().toURI()).getParent();
                    data = jarDir.resolve("snapshot.properties");
                    if (!Files.exists(data)) {
                        JFileChooser chooser = new JFileChooser();
                        chooser.setDialogTitle("Abra snapshot.properties da pasta jocas-modlist do seu modpack");
                        if (chooser.showOpenDialog(null) != JFileChooser.APPROVE_OPTION) return;
                        data = chooser.getSelectedFile().toPath();
                    }
                }
                Viewer viewer = new Viewer(Snapshot.load(data)); viewer.frame.setVisible(true);
                if (args.length == 3 && args[1].equals("--parent")) {
                    long pid = Long.parseLong(args[2]);
                    javax.swing.Timer timer = new javax.swing.Timer(2000, e -> {
                        if (!ProcessHandle.of(pid).map(ProcessHandle::isAlive).orElse(false)) {
                            ((javax.swing.Timer)e.getSource()).stop(); viewer.frame.dispose();
                        }
                    });
                    viewer.frame.addWindowListener(new WindowAdapter() {
                        @Override public void windowClosed(WindowEvent e) { timer.stop(); }
                    }); timer.start();
                }
            } catch (Exception e) { error(null, e); }
        });
    }
}
