import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.rmi.server.UnicastRemoteObject;
import java.util.HashMap;

public class RMIHistogram extends UnicastRemoteObject implements RemoteHistogram, Binder {
    private HashMap<Integer, int[]> histograms = new HashMap<>();
    private int nextHistogramId = 1;

    public RMIHistogram() throws RemoteException {
        super();
    }

    @Override
    public synchronized int createHistogram(int bins) throws RemoteException {
        int[] histogram = new int[bins];
        int histogramId = nextHistogramId++;
        histograms.put(histogramId, histogram);

        return histogramId;
    }

    @Override
    public synchronized void addToHistogram(int histogramID, int value) throws RemoteException {
        int[] histogram = histograms.get(histogramID);
        if (histogram != null) {
            if (value >= 0 && value < histogram.length) {
                histogram[value]++;
            } else throw new RemoteException("Nieprawidlowe id histogramu lub wartosc");
        } else throw new RemoteException("Nieprawidlowe id histogramu lub wartosc");
    }

    @Override
    public synchronized int[] getHistogram(int histogramID) throws RemoteException {
        int[] histogram = histograms.get(histogramID);
        if (histogram != null) {
            return histogram.clone();
        } else throw new RemoteException("Nieprawidlowe id histogramu");
    }

    @Override
    public void bind(String serviceName) {
        try {
            Registry registry = LocateRegistry.getRegistry(1099);
            java.rmi.Naming.rebind(serviceName, this);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}