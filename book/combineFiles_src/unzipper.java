
import java.util.*;
import java.io.*;
import java.util.zip.*;
public class unzipper
{
    public static void main(String[]args){
        ArrayList<String> probableZipFiles = new ArrayList<>();
        FileFilter filter = ((File file) -> {
                    String path = file.getAbsolutePath().toLowerCase();
                    String extensions[] = {"zip"};
                    for (int i = 0, n = extensions.length; i < n; i++) {
                        String extension = extensions[i];
                        if ((path.endsWith(extension) && (path.charAt(path.length() - extension.length() - 1)) == '.') && path.contains("euler")) {
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
                    probableZipFiles.add(file.getPath());
                }
            }
        }

        File[] files2 = new File("E:/Downloads/").listFiles(filter);
        //If this pathname does not denote a directory, then listFiles() returns null.
        if (files != null) {
            Arrays.sort(files2, (f1, f2) -> Long.valueOf(f2.lastModified()).compareTo(f1.lastModified()));
            for (File file : files2) {
                if (file.isFile()) {
                    probableZipFiles.add(file.getPath());
                }
            }
        }

        for(String e:probableZipFiles){
            extractFolder(e,"unzipped");
            extractFolder("unzipped/yourstuff.zip","unzipped");

            moveFolderContents("./unzipped/eulers-eureka-shawn-roach/arenaupload/","./");
            File f=new File("./unzipped/");
            deleteDir(f);
            f=new File(e);
            deleteDir(f);
        }

        moveFolderContents("../arenaupload/","./");

    }

    private static    void deleteDir(File file) {
        File[] contents = file.listFiles();
        if (contents != null) {
            for (File f : contents) {
                deleteDir(f);
            }
        }
        if(!file.delete()){
            System.out.println("failed to delete: "+file.getName());
        }
    }

    private static void moveFolderContents(String sourceFolderPath,String destinationFolderPath){
        File destinationFolder = new File(destinationFolderPath);
        File sourceFolder = new File(sourceFolderPath);

        if (!destinationFolder.exists())
        {
            destinationFolder.mkdirs();
        }

        // Check weather source exists and it is folder.
        if (sourceFolder.exists() && sourceFolder.isDirectory())
        {
            // Get list of the files and iterate over them
            File[] listOfFiles = sourceFolder.listFiles();

            if (listOfFiles != null)
            {
                for (File child : listOfFiles )
                {
                    // Move files to destination folder
                    child.renameTo(new File(destinationFolder + "\\" + child.getName()));
                }

                // Add if you want to delete the source folder 
                // sourceFolder.delete();
            }
        }
        else
        {
            System.out.println(sourceFolder + "  Folder does not exists");
        }
    }

    private static void extractFolder(String zipFile,String extractFolder) 
    {
        try
        {
            int BUFFER = 2048;
            File file = new File(zipFile);

            ZipFile zip = new ZipFile(file);
            String newPath = extractFolder;

            new File(newPath).mkdir();
            Enumeration zipFileEntries = zip.entries();

            // Process each entry
            while (zipFileEntries.hasMoreElements())
            {
                // grab a zip file entry
                ZipEntry entry = (ZipEntry) zipFileEntries.nextElement();
                String currentEntry = entry.getName();

                File destFile = new File(newPath, currentEntry);
                //destFile = new File(newPath, destFile.getName());
                File destinationParent = destFile.getParentFile();

                // create the parent directory structure if needed
                destinationParent.mkdirs();

                if (!entry.isDirectory())
                {
                    BufferedInputStream is = new BufferedInputStream(zip
                            .getInputStream(entry));
                    int currentByte;
                    // establish buffer for writing file
                    byte data[] = new byte[BUFFER];

                    // write the current file to disk
                    FileOutputStream fos = new FileOutputStream(destFile);
                    BufferedOutputStream dest = new BufferedOutputStream(fos,
                            BUFFER);

                    // read and write until last byte is encountered
                    while ((currentByte = is.read(data, 0, BUFFER)) != -1) {
                        dest.write(data, 0, currentByte);
                    }
                    dest.flush();
                    dest.close();
                    is.close();
                }

            }
            zip.close();
        }
        catch (Exception e) 
        {
            System.err.println("ERROR: "+e.getMessage());
        }

    }
}
