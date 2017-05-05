/**
 * Created by Shawn Roach on 4/17/2017.
 */

import java.util.*;
import java.io.*;
public class fileCombiner {


    public static void main(String[] args) {
        ArrayList<String> probableBlacklogFiles = new ArrayList<>();
        ArrayList<String> probableWhitelogFiles = new ArrayList<>();
        FileFilter filter = ((File file) -> {
                    String path = file.getAbsolutePath().toLowerCase();
                    String extensions[] = {"txt", "dat", "log"};
                    for (int i = 0, n = extensions.length; i < n; i++) {
                        String extension = extensions[i];
                        if ((path.endsWith(extension) && (path.charAt(path.length() - extension.length() - 1)) == '.')) {
                            return true;
                        }
                    }
                    return false;
                });
        File[] files = new File("./").listFiles(filter);
        //If this pathname does not denote a directory, then listFiles() returns null.
        if (files != null) {
            Arrays.sort(files, (f1, f2) -> Long.valueOf(f2.lastModified()).compareTo(f1.lastModified()));
            for (File file : files) {
                if (file.isFile()) {
                    if (file.getName().contains("black") && !file.getName().contains("OpeningTable"))
                        probableBlacklogFiles.add(file.getPath());
                }
            }
        }
        if (files != null) {
            Arrays.sort(files, (f1, f2) -> Long.valueOf(f2.lastModified()).compareTo(f1.lastModified()));
            for (File file : files) {
                if (file.isFile()) {
                    if (file.getName().contains("white") && !file.getName().contains("OpeningTable"))
                        probableWhitelogFiles.add(file.getPath());
                }
            }
        }

        try{
            PrintWriter bout = new PrintWriter(new BufferedWriter(new FileWriter("blackOpeningTable.txt", true)));
            bout.println();
            for(String e: probableBlacklogFiles){
                File f=new File(e);
                Scanner reader=new Scanner(f);
                while(reader.hasNext()){
                    String line=reader.nextLine();
                    bout.println(line);
                }
                reader.close();
                f.delete();
            }
            bout.close();

            PrintWriter wout = new PrintWriter(new BufferedWriter(new FileWriter("whiteOpeningTable.txt", true)));
            wout.println();
            for(String e: probableWhitelogFiles){
                File f=new File(e);
                Scanner reader=new Scanner(f);
                while(reader.hasNext()){
                    String line=reader.nextLine();
                    wout.println(line);
                }
                reader.close();
                f.delete();        
            }
            wout.close();
        }catch(IOException e){
            System.err.println(e);
        }
    }
}
