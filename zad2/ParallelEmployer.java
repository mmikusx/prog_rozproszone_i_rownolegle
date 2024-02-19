import java.util.List;
import java.util.concurrent.*;

public class ParallelEmployer implements Employer, ResultListener {
    private OrderInterface orderInterface;
    private final ExecutorService executorService;
    private final ConcurrentHashMap<Integer, Location> ongoingExplorations;
    private final ConcurrentHashMap<Location, Boolean> exploredLocations;
    private final CompletableFuture<Location> exitLocation;
    private final CountDownLatch completionLatch;
    private int activeExplorations;

    public ParallelEmployer() {
        this.executorService = Executors.newCachedThreadPool();
        this.ongoingExplorations = new ConcurrentHashMap<>();
        this.exploredLocations = new ConcurrentHashMap<>();
        this.exitLocation = new CompletableFuture<>();
        this.completionLatch = new CountDownLatch(1);
        this.activeExplorations = 0;
    }

    @Override
    public void setOrderInterface(OrderInterface order) {
        this.orderInterface = order;
        this.orderInterface.setResultListener(this);
    }

    @Override
    public Location findExit(Location startLocation, List<Direction> allowedDirections) {
        if (exploredLocations.putIfAbsent(startLocation, true) == null) {
            exploreConcurrently(startLocation, allowedDirections);

            try {
                return getLocation();
            } catch (InterruptedException | ExecutionException e) {
                e.printStackTrace();
            }
        }

        return null;
    }

    private Location getLocation() throws InterruptedException, ExecutionException {
        try {
            if (!completionLatch.await(5000, TimeUnit.MILLISECONDS)) {
                executorService.shutdownNow();
            }
        } finally {
            executorService.shutdown();
        }

        return exitLocation.get();
    }

    private void exploreConcurrently(Location currentLocation, List<Direction> directions) {
        synchronized (this) {
            if (!exitLocation.isDone() && !executorService.isShutdown()) {
                directions.stream().map(direction -> direction.step(currentLocation)).filter(neighbor ->
                exploredLocations.putIfAbsent(neighbor, true) == null).forEachOrdered(neighbor -> {
                    int orderID = orderInterface.order(neighbor);
                    ongoingExplorations.put(orderID, neighbor);

                    activeExplorations++;
                });
            }
        }
    }

    @Override
    public void result(Result result) {
        try {
            synchronized (this) {
                if (!exitLocation.isDone() && !executorService.isShutdown()) {
                    Location location = ongoingExplorations.get(result.orderID());
                    if (result.type() != LocationType.EXIT) {
                        executorService.submit(() -> exploreConcurrently(location, result.allowedDirections()));
                    } else {
                        executorService.shutdownNow();
                        exitLocation.complete(location);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            synchronized (this) {
                if (exitLocation.isDone() || executorService.isShutdown()) {
                    activeExplorations--;
                    completionLatch.countDown();
                }
            }
        }
    }
}